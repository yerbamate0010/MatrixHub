/**
 *   ESP32 SvelteKit
 *
 *   A simple, secure and extensible framework for IoT projects for ESP32 platforms
 *   with responsive Sveltekit front-end built with TailwindCSS and DaisyUI.
 *   https://github.com/theelims/ESP32-sveltekit
 *
 *   Copyright (C) 2018 - 2023 rjwats
 *   Copyright (C) 2023 - 2025 theelims
 *
 *   All Rights Reserved. This software may be modified and distributed under
 *   the terms of the LGPL v3 license. See the LICENSE file for details.
 **/

#include <wifi/WiFiSettingsService.h>

#include <WiFi.h>
#include <IPAddress.h>

#include <ArduinoJson.h>

#include <Preferences.h>
#include <cstring>

#include <services/RestartService.h>
#include <PsychicHttpsServer.h>
#include "BackoffCalculator.h"
#include "../../../src/system/health/wifi/WifiHealthTracker.h"
#include "../../../src/system/logging/Logging.h"
#include "../../../src/system/rtc/RtcConfig.h"

// Guard window after WiFi.begin() to avoid reconfiguring while the stack is busy.
// Reduced to 5s for faster failover when network is unavailable or credentials are wrong.
static constexpr uint32_t WIFI_CONNECT_GUARD_MS = 5000;
static constexpr const char* WIFI_FAST_TAG = "WiFiFast";

namespace {

struct ConnectedTransitionInfo {
    bool recovered = false;
    unsigned long offlineDurationMs = 0;
    bool ipChanged = false;
    IPAddress previousIp = INADDR_NONE;
    WiFiConnectivityState previousState = WiFiConnectivityState::StaConnecting;
};

struct RecoveryTransitionDecision {
    bool apply = false;
    bool dropped = false;
    wifi_mode_t targetMode = WIFI_MODE_STA;
    char reason[32]{0};
};

void refreshFastReconnectCache() {
    if (!WiFi.isConnected()) {
        return;
    }

    const uint8_t* bssid = WiFi.BSSID();
    const uint8_t channel = WiFi.channel();
    if (!bssid || channel == 0) {
        return;
    }

    RTC::networkState.update(channel,
                             bssid,
                             static_cast<uint32_t>(WiFi.localIP()),
                             static_cast<uint32_t>(WiFi.gatewayIP()),
                             static_cast<uint32_t>(WiFi.subnetMask()),
                             RTC::calculateSsidCrc(WiFi.SSID().c_str()));
}

const char* defaultReason(const char* reason, const char* fallback) {
    return (reason && reason[0] != '\0') ? reason : fallback;
}

void copyReason(char (&target)[32], const char* reason) {
    strlcpy(target, reason ? reason : "", sizeof(target));
}

bool isConfiguredStaticIp(const wifi_settings_t& network) {
    return network.staticIPConfig &&
           IPUtils::isSet(network.localIP) &&
           IPUtils::isSet(network.gatewayIP) &&
           IPUtils::isSet(network.subnetMask);
}

bool isZeroIp(const IPAddress& ip) {
    return static_cast<uint32_t>(ip) == 0;
}

}  // namespace

WiFiSettingsService::WiFiSettingsService(PsychicHttpServer *server,
                                         FS *fs,
                                         SecurityManager *securityManager)
    : _server(server),
      _securityManager(securityManager),
      _httpEndpoint(WiFiSettings::read,
                    WiFiSettings::update,
                    this,
                    server,
                    WIFI_SETTINGS_SERVICE_PATH,
                    securityManager,
                    AuthenticationPredicates::IS_ADMIN),
      _fsPersistence(WiFiSettings::read, WiFiSettings::update, this, fs, WIFI_SETTINGS_FILE),
      _lastConnectionAttempt(0) {
    addUpdateHandler([this](std::string_view originId) {
        (void)originId;
        const String currentHostname = _state.hostname;
        if (currentHostname != _lastAppliedHostname) {
            _lastAppliedHostname = currentHostname;
            if (_onHostnameChangeCallback) {
                _onHostnameChangeCallback();
            }
        }
        ESP_LOGI(SVK_TAG, "WiFi settings changed. Reconnecting shortly...");
        delayedReconnect();
        return StateHandlerResult::success();
    }, false);
}

void WiFiSettingsService::setAPSettingsService(APSettingsService *apSettingsService)
{
    _apSettingsService = apSettingsService;
}

void WiFiSettingsService::initWiFi()
{
    WiFi.mode(WIFI_MODE_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    _fsPersistence.readFromFS();
    _lastAppliedHostname = _state.hostname;
    _connectivityState = WiFiConnectivityState::StaConnecting;

    // Immediate first connection attempt (don't wait for loop delay)
    manageSTA();
}

void WiFiSettingsService::begin()
{
    _httpEndpoint.begin();
}

void WiFiSettingsService::delayedReconnect()
{
    RestartService::scheduleRestart();
}

void WiFiSettingsService::loop()
{
    const unsigned long currentMillis = millis();

    // Order matters: first observe the real STA state, then consume queued
    // recovery, then let policy react to the updated timestamps/flags.
    // This keeps the state machine deterministic during disconnect storms.
    observeConnectivity(currentMillis);
    (void)processRecoveryRequest();
    applyPolicy(currentMillis);

    const RuntimeSnapshot runtime = snapshotRuntimeState();
    unsigned long delay = WIFI_RECONNECTION_DELAY;
    if (runtime.retryBackoffActive) {
        delay = WIFI_UTILS::calculateBackoffDelay(runtime.backoffLevel,
                                                  WIFI_RETRY_BACKOFF_BASE_MS,
                                                  WIFI_RETRY_BACKOFF_MAX_MS);
    }

    if (!runtime.lastConnectionAttempt ||
        (unsigned long)(currentMillis - runtime.lastConnectionAttempt) >= delay)
    {
        portENTER_CRITICAL(&_runtimeStateLock);
        _lastConnectionAttempt = currentMillis;
        _retryBackoffActive = false;
        portEXIT_CRITICAL(&_runtimeStateLock);
        manageSTA();
    }
}

String WiFiSettingsService::getHostname()
{
    return _state.hostname;
}

String WiFiSettingsService::getIP()
{
    if (WiFi.isConnected())
    {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

bool WiFiSettingsService::isApModeConfigured() const
{
    return _state.staConnectionMode == (u_int8_t)STAConnectionMode::OFFLINE || !hasConfiguredNetworks();
}

bool WiFiSettingsService::hasConfiguredNetworks() const
{
    return !_state.wifiSettings.empty();
}

WiFiSettingsService::RuntimeSnapshot WiFiSettingsService::snapshotRuntimeState() const
{
    RuntimeSnapshot snapshot = {};

    // Copy every runtime field under one lock so diagnostics and policy work on
    // one consistent moment in time. The previous piecemeal reads were the core
    // reason recovery state and dashboard telemetry could drift apart.
    portENTER_CRITICAL(&_runtimeStateLock);
    snapshot.currentNetworkIndex = _currentNetworkIndex;
    snapshot.connectionAttempts = _connectionAttempts;
    snapshot.backoffLevel = _backoffLevel;
    snapshot.connectivityState = _connectivityState;
    snapshot.manualApOnly = _manualApOnly;
    snapshot.rescueApActive = _rescueApActive;
    snapshot.retryCycleCompleted = _retryCycleCompleted;
    snapshot.retryBackoffActive = _retryBackoffActive;
    snapshot.recoveryRequested = _recoveryRequested;
    snapshot.lastConnectionAttempt = _lastConnectionAttempt;
    snapshot.connectingSince = _connectingSince;
    snapshot.disconnectedSince = _disconnectedSince;
    snapshot.stableConnectedSince = _stableConnectedSince;
    snapshot.lastIpChangeMs = _lastIpChangeMs;
    snapshot.lastRescueEnterMs = _lastRescueEnterMs;
    snapshot.lastRescueExitMs = _lastRescueExitMs;
    snapshot.lastKnownStaIp = _lastKnownStaIp;
    copyReason(snapshot.rescueReason, _rescueReason);
    copyReason(snapshot.pendingRecoveryReason, _pendingRecoveryReason);
    copyReason(snapshot.lastRecoveryReason, _lastRecoveryReason);
    portEXIT_CRITICAL(&_runtimeStateLock);

    return snapshot;
}

bool WiFiSettingsService::requestRecovery(const char* reason)
{
    const RuntimeSnapshot runtime = snapshotRuntimeState();
    if (runtime.manualApOnly || _state.staConnectionMode == (u_int8_t)STAConnectionMode::OFFLINE) {
        ESP_LOGW(SVK_TAG, "Ignoring coordinated WiFi recovery request in manual AP-only mode");
        return false;
    }

    if (!hasConfiguredNetworks()) {
        ESP_LOGW(SVK_TAG, "Ignoring coordinated WiFi recovery request: no configured STA networks");
        return false;
    }

    if (WiFi.isConnected()) {
        return true;
    }

    // Queue recovery intent instead of applying WiFi operations directly here.
    // This keeps loop() as the single owner of state transitions and coalesces
    // bursts of identical requests into one recovery cycle with the first
    // reason preserved for diagnostics.
    char activeReason[32]{0};
    bool coalesced = false;
    portENTER_CRITICAL(&_runtimeStateLock);
    if (_recoveryRequested) {
        copyReason(activeReason, _pendingRecoveryReason);
        coalesced = true;
    } else {
        copyReason(_pendingRecoveryReason, defaultReason(reason, "external"));
        copyReason(activeReason, _pendingRecoveryReason);
        _recoveryRequested = true;
    }
    portEXIT_CRITICAL(&_runtimeStateLock);

    if (coalesced) {
        ESP_LOGI(SVK_TAG, "Coalescing WiFi recovery request: pending=%s new=%s",
             activeReason,
             defaultReason(reason, "external"));
        return true;
    }

    ESP_LOGW(SVK_TAG, "Queued coordinated WiFi recovery request: reason=%s", activeReason);
    return true;
}

WiFiConnectivityDiagnostics WiFiSettingsService::getConnectivityDiagnostics() const
{
    WiFiConnectivityDiagnostics diagnostics = {};
    // Build diagnostics from one runtime snapshot so the dashboard never sees
    // hybrid states like "connected" with stale disconnect timers, or rescue
    // flags that belong to a different transition than the reported state.
    const RuntimeSnapshot runtime = snapshotRuntimeState();
    const bool staConnected = WiFi.isConnected();
    const IPAddress staIp = staConnected ? WiFi.localIP() : IPAddress(INADDR_NONE);

    diagnostics.state = runtime.connectivityState;
    diagnostics.apLaunchMode = _apSettingsService ? _apSettingsService->getActiveLaunchMode() : ApLaunchMode::None;
    diagnostics.wifiMode = WiFi.getMode();
    diagnostics.staConnected = staConnected;
    diagnostics.rescueApActive = runtime.rescueApActive;
    diagnostics.disconnectedSinceMs = runtime.disconnectedSince;
    diagnostics.stableConnectedSinceMs = runtime.stableConnectedSince;
    diagnostics.lastIpChangeMs = runtime.lastIpChangeMs;
    diagnostics.lastRescueEnterMs = runtime.lastRescueEnterMs;
    diagnostics.lastRescueExitMs = runtime.lastRescueExitMs;
    diagnostics.lastDisconnectReason = SYSTEM::HEALTH::WifiHealthTracker::getHealth().lastDisconnectReason;
    diagnostics.apStationCount = static_cast<uint8_t>(WiFi.softAPgetStationNum());
    diagnostics.apIp = WiFi.softAPIP();
    diagnostics.staIp = staIp;
    copyReason(diagnostics.rescueReason, runtime.rescueReason);
    copyReason(diagnostics.lastRecoveryReason, runtime.lastRecoveryReason);

    const size_t diagnosticsNetworkIndex = WIFI_DIAGNOSTICS::selectNetworkIndex(
        _state.wifiSettings,
        diagnostics.staConnected ? WiFi.SSID().c_str() : nullptr,
        diagnostics.staConnected,
        runtime.currentNetworkIndex,
        [](const wifi_settings_t& network) {
            return network.ssid.c_str();
        });

    if (diagnosticsNetworkIndex != WIFI_DIAGNOSTICS::kNoNetworkIndex) {
        const wifi_settings_t& primary = _state.wifiSettings[diagnosticsNetworkIndex];
        diagnostics.savedStaticIpConfigured = isConfiguredStaticIp(primary);
        if (diagnostics.savedStaticIpConfigured) {
            diagnostics.savedStaticIp = primary.localIP;
            diagnostics.savedStaticIpMatches =
                diagnostics.staConnected && (primary.localIP == diagnostics.staIp);
        }
    }

    bool httpsRunning = false;
#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
    if (_server && _server->server) {
        auto* secureServer = static_cast<PsychicHttpsServer*>(_server);
        httpsRunning = secureServer->ssl_config.port_secure == 443;
    }
#endif

    diagnostics.portForwardingReady =
        diagnostics.staConnected &&
        diagnostics.savedStaticIpConfigured &&
        diagnostics.savedStaticIpMatches &&
        httpsRunning &&
        !runtime.rescueApActive &&
        !runtime.manualApOnly;

    return diagnostics;
}

void WiFiSettingsService::observeConnectivity(unsigned long nowMs)
{
    if (WiFi.isConnected()) {
        handleConnected(nowMs);
        return;
    }

    portENTER_CRITICAL(&_runtimeStateLock);
    if (_stableConnectedSince != 0) {
        _stableConnectedSince = 0;
    }

    if (_disconnectedSince == 0) {
        _disconnectedSince = nowMs;
    }
    portEXIT_CRITICAL(&_runtimeStateLock);
}

void WiFiSettingsService::handleConnected(unsigned long nowMs)
{
    const IPAddress currentIp = WiFi.localIP();
    ConnectedTransitionInfo transition = {};

    // A successful STA connection is the point where we collapse any stale
    // recovery intent, clear offline timers and restart the retry machine from
    // a known-good baseline. Target behavior after recovery is simple:
    // connected STA, no pending recovery, no stale disconnect window.
    portENTER_CRITICAL(&_runtimeStateLock);
    transition.previousState = _connectivityState;
    if (_disconnectedSince != 0) {
        transition.recovered = true;
        transition.offlineDurationMs = static_cast<unsigned long>(nowMs - _disconnectedSince);
        _disconnectedSince = 0;
    }

    if (_stableConnectedSince == 0) {
        _stableConnectedSince = nowMs;
    }

    _connectingSince = 0;
    _currentNetworkIndex = 0;
    _connectionAttempts = 0;
    _retryBackoffActive = false;
    _retryCycleCompleted = false;
    _backoffLevel = 0;
    _recoveryRequested = false;
    _pendingRecoveryReason[0] = '\0';

    if (_lastKnownStaIp != currentIp) {
        transition.ipChanged = true;
        transition.previousIp = _lastKnownStaIp;
        _lastKnownStaIp = currentIp;
        _lastIpChangeMs = nowMs;
    }
    portEXIT_CRITICAL(&_runtimeStateLock);

    if (transition.recovered) {
        ESP_LOGI(SVK_TAG, "STA recovered after %lu ms offline (state=%s)",
             transition.offlineDurationMs,
             WIFI_CONNECTIVITY_POLICY::stateName(transition.previousState));
    }

    refreshFastReconnectCache();

    if (transition.ipChanged) {
        if (!isZeroIp(transition.previousIp)) {
            ESP_LOGI(SVK_TAG, "STA IP changed: %s -> %s",
                 transition.previousIp.toString().c_str(),
                 currentIp.toString().c_str());
        } else {
            ESP_LOGI(SVK_TAG, "STA IP acquired: %s", currentIp.toString().c_str());
        }
    }
}

void WiFiSettingsService::applyPolicy(unsigned long nowMs)
{
    const RuntimeSnapshot runtime = snapshotRuntimeState();
    WiFiConnectivityPolicyInput input = {};
    input.hasConfiguredNetworks = hasConfiguredNetworks();
    input.staConnected = WiFi.isConnected();
    input.rescueApActive = runtime.rescueApActive;
    input.manualApOnly = runtime.manualApOnly;
    input.completedNetworkCycle = runtime.retryCycleCompleted;
    input.nowMs = nowMs;
    input.disconnectedSinceMs = runtime.disconnectedSince;
    input.stableConnectedSinceMs = runtime.stableConnectedSince;

    // Policy decides only high-level mode transitions: stay connecting,
    // enter rescue AP_STA after long offline time, or leave rescue once STA is
    // stable again. The exact thresholds live in WifiConnectivityPolicy.
    const WiFiConnectivityPolicyDecision decision = WIFI_CONNECTIVITY_POLICY::evaluate(input);

    if (decision.enterRescueAp) {
        if (!enterRescueApSta("sta_offline_timeout")) {
            portENTER_CRITICAL(&_runtimeStateLock);
            _connectivityState = WiFiConnectivityState::StaConnecting;
            portEXIT_CRITICAL(&_runtimeStateLock);
            return;
        }
    }

    if (decision.exitRescueAp) {
        exitRescueApSta("sta_stable");
    }

    portENTER_CRITICAL(&_runtimeStateLock);
    _connectivityState = decision.nextState;
    portEXIT_CRITICAL(&_runtimeStateLock);
}

bool WiFiSettingsService::processRecoveryRequest()
{
    RecoveryTransitionDecision decision = {};
    const bool staConnected = WiFi.isConnected();

    // Only loop() is allowed to consume queued recovery and reset the retry
    // machine. We copy the decision under lock, clear the request there, and
    // only then call into WiFi outside the critical section. That avoids both
    // lost requests and long lock hold times around stack operations.
    portENTER_CRITICAL(&_runtimeStateLock);
    if (!_recoveryRequested) {
        portEXIT_CRITICAL(&_runtimeStateLock);
        return false;
    }

    if (staConnected) {
        _recoveryRequested = false;
        _pendingRecoveryReason[0] = '\0';
        portEXIT_CRITICAL(&_runtimeStateLock);
        return false;
    }

    if (_manualApOnly) {
        decision.dropped = true;
        copyReason(decision.reason, _pendingRecoveryReason);
        _recoveryRequested = false;
        _pendingRecoveryReason[0] = '\0';
        portEXIT_CRITICAL(&_runtimeStateLock);
        ESP_LOGW(SVK_TAG, "Dropping queued WiFi recovery request in manual AP-only mode");
        return false;
    }

    decision.apply = true;
    decision.targetMode = _rescueApActive ? WIFI_AP_STA : WIFI_STA;
    copyReason(decision.reason, _pendingRecoveryReason);
    copyReason(_lastRecoveryReason, _pendingRecoveryReason);

    _connectingSince = 0;
    _lastConnectionAttempt = 0;
    _currentNetworkIndex = 0;
    _connectionAttempts = 0;
    _retryBackoffActive = false;
    _retryCycleCompleted = false;
    _backoffLevel = 0;
    _recoveryRequested = false;
    _pendingRecoveryReason[0] = '\0';
    portEXIT_CRITICAL(&_runtimeStateLock);

    ESP_LOGW(SVK_TAG, "Applying coordinated WiFi recovery: reason=%s target_mode=%d rescue=%d",
         decision.reason,
         static_cast<int>(decision.targetMode),
         decision.targetMode == WIFI_AP_STA ? 1 : 0);

    WiFi.disconnect(false);
    WiFi.mode(decision.targetMode);

    return true;
}

void WiFiSettingsService::manageSTA()
{
    const RuntimeSnapshot runtime = snapshotRuntimeState();

    if (runtime.manualApOnly)
    {
        return;
    }

    if (_state.staConnectionMode == (u_int8_t)STAConnectionMode::OFFLINE)
    {
        ESP_LOGI(SVK_TAG, "STA mode disabled. Switching to manual AP-only mode.");
        switchToAPMode("sta_offline");
        return;
    }

    if (_state.wifiSettings.empty())
    {
        ESP_LOGI(SVK_TAG, "No STA networks configured. Switching to manual AP-only mode.");
        switchToAPMode("no_saved_networks");
        return;
    }

    if (WiFi.isConnected())
    {
        return;
    }

    if (runtime.connectingSince != 0 &&
        (unsigned long)(millis() - runtime.connectingSince) < WIFI_CONNECT_GUARD_MS)
    {
        return;
    }

    // Once the guard window has elapsed, let the local retry state machine own
    // the next STA attempt instead of depending on a narrow wl_status_t list.
    // Real disconnect paths can surface different transient statuses, but the
    // reconnect rule we actually care about is simply: still offline, try next.
    connectToWiFi();
}

void WiFiSettingsService::connectToWiFi()
{
    if (_state.wifiSettings.empty())
    {
        return;
    }

    const size_t networkCount = _state.wifiSettings.size();
    size_t networkIndex = 0;
    uint8_t attemptNumber = 0;
    bool rescueApActive = false;

    portENTER_CRITICAL(&_runtimeStateLock);
    while (_currentNetworkIndex < networkCount && _connectionAttempts >= WIFI_MAX_ATTEMPTS_PER_NETWORK)
    {
        _connectionAttempts = 0;
        _currentNetworkIndex++;
    }

    if (_currentNetworkIndex >= networkCount)
    {
        unsigned long backoffDelay = WIFI_UTILS::calculateBackoffDelay(_backoffLevel,
                                                                       WIFI_RETRY_BACKOFF_BASE_MS,
                                                                       WIFI_RETRY_BACKOFF_MAX_MS);
        const uint8_t loggedBackoffLevel = _backoffLevel;

        _retryCycleCompleted = true;
        _currentNetworkIndex = 0;
        _connectionAttempts = 0;
        _lastConnectionAttempt = millis();
        _retryBackoffActive = true;

        if (_backoffLevel < 4) {
            _backoffLevel++;
        }
        portEXIT_CRITICAL(&_runtimeStateLock);

        ESP_LOGW(SVK_TAG, "All STA networks failed. Retrying in %u seconds... (level %u)",
                 static_cast<unsigned>(backoffDelay / 1000),
                 loggedBackoffLevel);

        return;
    }

    networkIndex = _currentNetworkIndex;
    _connectionAttempts++;
    attemptNumber = _connectionAttempts;
    rescueApActive = _rescueApActive;
    portEXIT_CRITICAL(&_runtimeStateLock);

    wifi_settings_t &network = _state.wifiSettings[networkIndex];

    ESP_LOGD(SVK_TAG,
             "Connecting to network: %s (attempt %u/%u, rescue=%d)",
             network.ssid.c_str(),
             attemptNumber,
             WIFI_MAX_ATTEMPTS_PER_NETWORK,
             rescueApActive ? 1 : 0);

    if (_onActivityCallback) {
        _onActivityCallback();
    }

    configureNetwork(network);
    portENTER_CRITICAL(&_runtimeStateLock);
    _connectingSince = millis();
    _connectivityState = _rescueApActive ? WiFiConnectivityState::RescueApSta
                                         : WiFiConnectivityState::StaConnecting;

    if (_connectionAttempts >= WIFI_MAX_ATTEMPTS_PER_NETWORK)
    {
        _connectionAttempts = WIFI_MAX_ATTEMPTS_PER_NETWORK;
    }
    portEXIT_CRITICAL(&_runtimeStateLock);
}

void WiFiSettingsService::configureNetwork(wifi_settings_t &network)
{
    if (network.staticIPConfig)
    {
        WiFi.config(network.localIP, network.gatewayIP, network.subnetMask, network.dnsIP1, network.dnsIP2);
    }
    else
    {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }
    WiFi.setHostname(_state.hostname.c_str());

    WiFi.mode(_rescueApActive ? WIFI_AP_STA : WIFI_MODE_STA);

    const uint32_t targetSsidCrc = RTC::calculateSsidCrc(network.ssid.c_str());

    if (RTC::networkState.matchesSsid(targetSsidCrc)) {
        LOG::Logging::log(ESP_LOG_INFO, WIFI_FAST_TAG,
                          "Fast-connect using cached BSSID (ch=%u)",
                          RTC::networkState.channel);
        WiFi.begin(network.ssid.c_str(),
                   network.password.c_str(),
                   RTC::networkState.channel,
                   RTC::networkState.bssid,
                   true);
        RTC::networkState.invalidate();
    } else {
        WiFi.begin(network.ssid.c_str(), network.password.c_str());
    }

    // Start from a less aggressive STA power level; thermal monitor can still
    // tighten or relax this later based on actual board temperature.
    WiFi.setTxPower(WIFI_POWER_15dBm);
    WiFi.setSleep(false);
}

void WiFiSettingsService::switchToAPMode(const char* reason)
{
    const RuntimeSnapshot runtime = snapshotRuntimeState();
    if (runtime.manualApOnly) {
        return;
    }

    ESP_LOGI(SVK_TAG, "Switching to manual AP-only mode: reason=%s", defaultReason(reason, "manual"));
    WiFi.setHostname(_state.hostname.c_str());

    if (!_apSettingsService ||
        !_apSettingsService->startAccessPoint(ApLaunchMode::ManualApOnly))
    {
        // Only disable STA recovery after SoftAP startup succeeds. Otherwise a
        // failed AP transition would leave the device in a "manual AP-only"
        // runtime state without any working AP to reach it.
        ESP_LOGE(SVK_TAG, "Manual AP-only transition aborted: SoftAP failed to start");
        return;
    }

    // Manual AP-only is an operator-controlled fallback: once the AP is up we
    // can safely stop STA retry loops and clear any stale queued recovery work.
    portENTER_CRITICAL(&_runtimeStateLock);
    _manualApOnly = true;
    _rescueApActive = false;
    _connectivityState = WiFiConnectivityState::ManualApOnly;
    _currentNetworkIndex = 0;
    _connectionAttempts = 0;
    _connectingSince = 0;
    _retryBackoffActive = false;
    _retryCycleCompleted = false;
    _backoffLevel = 0;
    _recoveryRequested = false;
    _pendingRecoveryReason[0] = '\0';
    portEXIT_CRITICAL(&_runtimeStateLock);
}

bool WiFiSettingsService::enterRescueApSta(const char* reason)
{
    const RuntimeSnapshot runtime = snapshotRuntimeState();
    if (runtime.rescueApActive) {
        return true;
    }

    if (runtime.manualApOnly || !_apSettingsService) {
        return false;
    }

    // Rescue AP_STA is the autonomous fallback for prolonged STA outage. The
    // target behavior is: keep trying STA in the background, but also expose an
    // AP so the dashboard and recovery tools stay reachable in the field.
    char rescueReason[32]{0};
    copyReason(rescueReason, defaultReason(reason, "sta_offline_timeout"));
    ESP_LOGW(SVK_TAG, "Entering rescue AP_STA mode: reason=%s", rescueReason);

    WiFi.setHostname(_state.hostname.c_str());
    if (!_apSettingsService->startAccessPoint(ApLaunchMode::RescueApSta)) {
        ESP_LOGE(SVK_TAG, "Failed to start rescue AP_STA");
        return false;
    }

    portENTER_CRITICAL(&_runtimeStateLock);
    copyReason(_rescueReason, rescueReason);
    _rescueApActive = true;
    _manualApOnly = false;
    _connectivityState = WiFiConnectivityState::RescueApSta;
    _lastRescueEnterMs = millis();
    _lastConnectionAttempt = 0;
    _connectingSince = 0;
    _retryBackoffActive = false;
    _retryCycleCompleted = false;
    _backoffLevel = 0;
    portEXIT_CRITICAL(&_runtimeStateLock);
    return true;
}

void WiFiSettingsService::exitRescueApSta(const char* reason)
{
    const RuntimeSnapshot runtime = snapshotRuntimeState();
    if (!runtime.rescueApActive) {
        return;
    }

    // Exit rescue only after policy has observed a stable STA window. This
    // avoids flapping between AP_STA and pure STA when the upstream network is
    // still unstable right after reconnect.
    ESP_LOGI(SVK_TAG, "Exiting rescue AP_STA mode: reason=%s", defaultReason(reason, "sta_stable"));

    if (_apSettingsService) {
        _apSettingsService->stopAccessPoint();
    }

    portENTER_CRITICAL(&_runtimeStateLock);
    _rescueApActive = false;
    _connectivityState = WiFi.isConnected() ? WiFiConnectivityState::StaConnected
                                            : WiFiConnectivityState::StaConnecting;
    _lastRescueExitMs = millis();
    portEXIT_CRITICAL(&_runtimeStateLock);
}
