#include <unity.h>

#include <atomic>
#include <cstdarg>

#include "../../lib/ArduinoJson/src/ArduinoJson.h"
#include "../../test/stubs/esp_log.h"
#include "../../test/stubs/WiFi.h"

#define private public
#define protected public
#include "../../lib/framework/wifi/WiFiSettingsService.h"
#include "../../lib/framework/utils/SettingValue.cpp"
#include "../../lib/framework/wifi/WifiConnectivityPolicy.cpp"
#include "../../lib/framework/wifi/WiFiSettingsService.cpp"
#undef private
#undef protected

#include "../../src/system/rtc/types/RtcNetworkState.h"
#include "../../src/system/health/wifi/WifiHealthTracker.h"
#include "../../src/system/logging/Logging.h"
// Native build now provides a shared inline RestartService fallback, so this
// test no longer needs to declare its own local restart stubs.
#include "../../lib/framework/services/RestartService.h"

WiFiClass WiFi;
SemaphoreHandle_t g_fsMutex = nullptr;

update_handler_id_t StateUpdateHandlerInfo::currentUpdatedHandlerId = 0;
hook_handler_id_t StateHookHandlerInfo::currentHookHandlerId = 0;

namespace RTC {
RtcNetworkState networkState;
}

namespace SYSTEM::HEALTH {
const WiFiHealth& WifiHealthTracker::getHealth() {
    static WiFiHealth health{};
    return health;
}
}

namespace LOG {
Settings Logging::_settings{ESP_LOG_INFO};

void Logging::begin(const Settings& settings) { _settings = settings; }
void Logging::setLevel(esp_log_level_t level) { _settings.level = level; }
Settings Logging::settings() { return _settings; }
bool Logging::isEnabled(esp_log_level_t level) { return level <= _settings.level; }
void Logging::log(esp_log_level_t level, const char* tag, const char* fmt, ...) {
    (void)level;
    (void)tag;
    (void)fmt;
}
void Logging::clearBuffer() {}
const char* Logging::levelToString(esp_log_level_t level) {
    (void)level;
    return "INFO";
}
esp_log_level_t Logging::stringToLevel(std::string_view name, esp_log_level_t fallback) {
    (void)name;
    return fallback;
}
void Logging::logStackHwm(const char* taskName, uint32_t stackSize) {
    (void)taskName;
    (void)stackSize;
}
void Logging::logSection(const char* title) {
    (void)title;
}
void Logging::suppressNoisyModules() {}
}

namespace {
bool s_apStartShouldSucceed = true;
int s_apStartCalls = 0;
int s_apStopCalls = 0;
ApLaunchMode s_lastApStartMode = ApLaunchMode::None;
}

APSettingsService::APSettingsService(PsychicHttpServer* server,
                                     FS* fs,
                                     SecurityManager* securityManager)
    : _server(server),
      _securityManager(securityManager),
      _httpEndpoint(APSettings::read,
                    APSettings::update,
                    this,
                    server,
                    AP_SETTINGS_SERVICE_PATH,
                    securityManager,
                    AuthenticationPredicates::IS_ADMIN,
                    APSettings::validate),
      _fsPersistence(APSettings::read, APSettings::update, this, fs, AP_SETTINGS_FILE),
      _dnsServer(nullptr) {
    // Native tests replace the runtime AP bring-up logic below, so this
    // lightweight constructor only needs to satisfy object layout/linking.
}

bool APSettingsService::startAccessPoint(ApLaunchMode mode) {
    ++s_apStartCalls;
    s_lastApStartMode = mode;
    _launchMode = s_apStartShouldSucceed ? mode : ApLaunchMode::None;
    return s_apStartShouldSucceed;
}

void APSettingsService::stopAccessPoint() {
    ++s_apStopCalls;
    _launchMode = ApLaunchMode::None;
}

namespace {

wifi_settings_t configuredNetwork() {
    wifi_settings_t network{};
    network.ssid = "matrixhub";
    network.password = "secret";
    network.staticIPConfig = false;
    network.localIP = INADDR_NONE;
    network.gatewayIP = INADDR_NONE;
    network.subnetMask = INADDR_NONE;
    network.dnsIP1 = INADDR_NONE;
    network.dnsIP2 = INADDR_NONE;
    return network;
}

void configureService(WiFiSettingsService& service) {
    service._state = WiFiSettings{};
    service._state.hostname = "device";
    service._state.staConnectionMode = static_cast<uint8_t>(STAConnectionMode::ONLINE);
    service._state.wifiSettings.push_back(configuredNetwork());
}

}  // namespace

void setUp(void) {
    TEST_STUBS::WIFI::reset();
    TEST_STUBS::ARDUINO::millisValue = 0;
    s_apStartShouldSucceed = true;
    s_apStartCalls = 0;
    s_apStopCalls = 0;
    s_lastApStartMode = ApLaunchMode::None;
}

void tearDown(void) {}

void test_requestRecovery_coalesces_pending_reason_and_process_preserves_first_reason() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    configureService(service);
    TEST_STUBS::WIFI::connected = false;
    TEST_STUBS::WIFI::status = WL_DISCONNECTED;

    TEST_ASSERT_TRUE(service.requestRecovery("api/manual"));
    TEST_ASSERT_TRUE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("api/manual", service._pendingRecoveryReason);

    TEST_ASSERT_TRUE(service.requestRecovery("health_pulse"));
    TEST_ASSERT_TRUE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("api/manual", service._pendingRecoveryReason);

    TEST_ASSERT_TRUE(service.processRecoveryRequest());
    TEST_ASSERT_FALSE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("", service._pendingRecoveryReason);
    TEST_ASSERT_EQUAL_STRING("api/manual", service._lastRecoveryReason);
    TEST_ASSERT_EQUAL(1, TEST_STUBS::WIFI::disconnectCalls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WIFI_STA), static_cast<int>(TEST_STUBS::WIFI::mode));
}

void test_handleConnected_clears_stale_pending_recovery_reason() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    configureService(service);
    TEST_STUBS::WIFI::connected = true;
    TEST_STUBS::WIFI::status = WL_CONNECTED;
    TEST_STUBS::WIFI::localIp = static_cast<uint32_t>(IPAddress(192, 168, 1, 77));

    service._recoveryRequested = true;
    strlcpy(service._pendingRecoveryReason, "stale", sizeof(service._pendingRecoveryReason));
    service._disconnectedSince = 100;

    service.handleConnected(250);

    TEST_ASSERT_FALSE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("", service._pendingRecoveryReason);
    TEST_ASSERT_EQUAL(0UL, service._disconnectedSince);
    TEST_ASSERT_TRUE(service._lastKnownStaIp == IPAddress(192, 168, 1, 77));
    TEST_ASSERT_EQUAL(250UL, service._lastIpChangeMs);
}

void test_switchToAPMode_keeps_recovery_alive_when_softap_start_fails() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    APSettingsService apService(&server, &fs, &security);
    configureService(service);
    service.setAPSettingsService(&apService);
    service._connectivityState = WiFiConnectivityState::StaConnecting;
    service._recoveryRequested = true;
    strlcpy(service._pendingRecoveryReason, "queued", sizeof(service._pendingRecoveryReason));
    s_apStartShouldSucceed = false;

    service.switchToAPMode("no_saved_networks");

    TEST_ASSERT_EQUAL(1, s_apStartCalls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ApLaunchMode::ManualApOnly),
                          static_cast<int>(s_lastApStartMode));
    TEST_ASSERT_FALSE(service._manualApOnly);
    TEST_ASSERT_FALSE(service._rescueApActive);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::StaConnecting),
                          static_cast<int>(service._connectivityState));
    TEST_ASSERT_TRUE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("queued", service._pendingRecoveryReason);
}

void test_manageSTA_retries_even_when_wifi_status_is_transient() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    configureService(service);
    TEST_STUBS::WIFI::connected = false;
    TEST_STUBS::WIFI::status = static_cast<wl_status_t>(7);
    TEST_STUBS::ARDUINO::millisValue = 1234;

    service.manageSTA();

    TEST_ASSERT_EQUAL(1, TEST_STUBS::WIFI::beginCalls);
    TEST_ASSERT_EQUAL_STRING("matrixhub", TEST_STUBS::WIFI::lastBeginSsid.c_str());
    TEST_ASSERT_EQUAL_STRING("secret", TEST_STUBS::WIFI::lastBeginPassword.c_str());
    TEST_ASSERT_EQUAL(1234UL, service._connectingSince);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::StaConnecting),
                          static_cast<int>(service._connectivityState));
}

void test_switchToAPMode_success_clears_recovery_state_and_enters_manual_ap_only() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    APSettingsService apService(&server, &fs, &security);
    configureService(service);
    service.setAPSettingsService(&apService);
    service._recoveryRequested = true;
    strlcpy(service._pendingRecoveryReason, "queued", sizeof(service._pendingRecoveryReason));
    service._connectionAttempts = 2;
    service._backoffLevel = 3;
    service._retryBackoffActive = true;

    service.switchToAPMode("no_saved_networks");

    TEST_ASSERT_EQUAL(1, s_apStartCalls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ApLaunchMode::ManualApOnly),
                          static_cast<int>(s_lastApStartMode));
    TEST_ASSERT_TRUE(service._manualApOnly);
    TEST_ASSERT_FALSE(service._rescueApActive);
    TEST_ASSERT_FALSE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("", service._pendingRecoveryReason);
    TEST_ASSERT_EQUAL(0, service._connectionAttempts);
    TEST_ASSERT_EQUAL(0, service._backoffLevel);
    TEST_ASSERT_FALSE(service._retryBackoffActive);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::ManualApOnly),
                          static_cast<int>(service._connectivityState));
}

void test_enterRescueApSta_sets_state_and_starts_ap_sta_fallback() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    APSettingsService apService(&server, &fs, &security);
    configureService(service);
    service.setAPSettingsService(&apService);
    TEST_STUBS::ARDUINO::millisValue = 5000;

    const bool entered = service.enterRescueApSta("health");

    TEST_ASSERT_TRUE(entered);
    TEST_ASSERT_EQUAL(1, s_apStartCalls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ApLaunchMode::RescueApSta),
                          static_cast<int>(s_lastApStartMode));
    TEST_ASSERT_TRUE(service._rescueApActive);
    TEST_ASSERT_FALSE(service._manualApOnly);
    TEST_ASSERT_EQUAL_STRING("health", service._rescueReason);
    TEST_ASSERT_EQUAL(5000UL, service._lastRescueEnterMs);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::RescueApSta),
                          static_cast<int>(service._connectivityState));
    TEST_ASSERT_EQUAL_STRING("device", TEST_STUBS::WIFI::hostname);
}

void test_processRecoveryRequest_uses_ap_sta_mode_when_rescue_ap_is_active() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    configureService(service);
    TEST_STUBS::WIFI::connected = false;
    TEST_STUBS::WIFI::status = WL_DISCONNECTED;
    TEST_STUBS::WIFI::mode = WIFI_MODE_STA;
    service._rescueApActive = true;
    service._recoveryRequested = true;
    strlcpy(service._pendingRecoveryReason, "health", sizeof(service._pendingRecoveryReason));

    TEST_ASSERT_TRUE(service.processRecoveryRequest());

    TEST_ASSERT_EQUAL(1, TEST_STUBS::WIFI::disconnectCalls);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WIFI_AP_STA), static_cast<int>(TEST_STUBS::WIFI::mode));
    TEST_ASSERT_FALSE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("", service._pendingRecoveryReason);
    TEST_ASSERT_EQUAL_STRING("health", service._lastRecoveryReason);
}

void test_processRecoveryRequest_drops_request_in_manual_ap_only_mode() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    configureService(service);
    TEST_STUBS::WIFI::connected = false;
    TEST_STUBS::WIFI::status = WL_DISCONNECTED;
    service._manualApOnly = true;
    service._recoveryRequested = true;
    strlcpy(service._pendingRecoveryReason, "manual", sizeof(service._pendingRecoveryReason));

    TEST_ASSERT_FALSE(service.processRecoveryRequest());

    TEST_ASSERT_EQUAL(0, TEST_STUBS::WIFI::disconnectCalls);
    TEST_ASSERT_FALSE(service._recoveryRequested);
    TEST_ASSERT_EQUAL_STRING("", service._pendingRecoveryReason);
}

void test_exitRescueApSta_stops_ap_and_returns_to_sta_connected_state() {
    PsychicHttpServer server;
    SecurityManager security;
    FS fs;
    WiFiSettingsService service(&server, &fs, &security);
    APSettingsService apService(&server, &fs, &security);
    configureService(service);
    service.setAPSettingsService(&apService);
    service._rescueApActive = true;
    service._connectivityState = WiFiConnectivityState::RescueApSta;
    TEST_STUBS::WIFI::connected = true;
    TEST_STUBS::WIFI::status = WL_CONNECTED;
    TEST_STUBS::ARDUINO::millisValue = 9000;

    service.exitRescueApSta("sta_stable");

    TEST_ASSERT_EQUAL(1, s_apStopCalls);
    TEST_ASSERT_FALSE(service._rescueApActive);
    TEST_ASSERT_EQUAL(9000UL, service._lastRescueExitMs);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::StaConnected),
                          static_cast<int>(service._connectivityState));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_requestRecovery_coalesces_pending_reason_and_process_preserves_first_reason);
    RUN_TEST(test_handleConnected_clears_stale_pending_recovery_reason);
    RUN_TEST(test_switchToAPMode_keeps_recovery_alive_when_softap_start_fails);
    RUN_TEST(test_manageSTA_retries_even_when_wifi_status_is_transient);
    RUN_TEST(test_switchToAPMode_success_clears_recovery_state_and_enters_manual_ap_only);
    RUN_TEST(test_enterRescueApSta_sets_state_and_starts_ap_sta_fallback);
    RUN_TEST(test_processRecoveryRequest_uses_ap_sta_mode_when_rescue_ap_is_active);
    RUN_TEST(test_processRecoveryRequest_drops_request_in_manual_ap_only_mode);
    RUN_TEST(test_exitRescueApSta_stops_ap_and_returns_to_sta_connected_state);
    return UNITY_END();
}
