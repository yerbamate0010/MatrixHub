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

#include <wifi/APSettingsService.h>
#include <services/RestartService.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <new>

namespace {

bool isZeroIp(const IPAddress &ip) {
    return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

String resolveApDnsHostname() {
    const char* hostname = WiFi.getHostname();
    if (!hostname || hostname[0] == '\0') {
        return String();
    }

    String dnsName(hostname);
    dnsName += ".local";
    return dnsName;
}

}  // namespace

APSettingsService::APSettingsService(PsychicHttpServer *server,
                                     FS *fs,
                                     SecurityManager *securityManager) : _server(server),
                                                                         _securityManager(securityManager),
                                                                         _httpEndpoint(APSettings::read, APSettings::update, this, server, AP_SETTINGS_SERVICE_PATH, securityManager, AuthenticationPredicates::IS_ADMIN, APSettings::validate),
                                                                         _fsPersistence(APSettings::read, APSettings::update, this, fs, AP_SETTINGS_FILE),
                                                                         _dnsServer(nullptr)
{
    addUpdateHandler([&](std::string_view originId) {
        (void)originId;
        // HttpEndpoint triggers update handlers BEFORE sending the HTTP response.
        // Schedule restart via central service - timer allows response to be sent first.
        ESP_LOGI(SVK_TAG, "AP settings changed. Scheduling restart...");
        RestartService::scheduleRestart();
        return StateHandlerResult::success();
    }, false);
}

void APSettingsService::begin()
{
    _httpEndpoint.begin();
    _fsPersistence.readFromFS();
}

void APSettingsService::loop()
{
    handleDNS();
}

const char* APSettingsService::apLaunchModeName(ApLaunchMode mode)
{
    switch (mode) {
        case ApLaunchMode::None:
            return "none";
        case ApLaunchMode::ManualApOnly:
            return "manual";
        case ApLaunchMode::RescueApSta:
            return "rescue";
        default:
            return "unknown";
    }
}

void APSettingsService::ensureLoadedState()
{
    // NOTE: WiFiSettingsService may call AP setup very early during boot
    // before APSettingsService::begin() has loaded values from FS.
    if (_state.ssid.isEmpty() || isZeroIp(_state.localIP))
    {
        _fsPersistence.readFromFS();

        if (_state.ssid.isEmpty())
        {
            _state.ssid = SettingValue::format(FACTORY_AP_SSID);
        }
        if (_state.password.isEmpty())
        {
            _state.password = FACTORY_AP_PASSWORD;
        }
        if (_state.channel == 0)
        {
            _state.channel = FACTORY_AP_CHANNEL;
        }
        if (_state.maxClients == 0)
        {
            _state.maxClients = FACTORY_AP_MAX_CLIENTS;
        }
        if (isZeroIp(_state.localIP))
        {
            _state.localIP.fromString(FACTORY_AP_LOCAL_IP);
        }
        if (isZeroIp(_state.gatewayIP))
        {
            _state.gatewayIP.fromString(FACTORY_AP_GATEWAY_IP);
        }
        if (isZeroIp(_state.subnetMask))
        {
            _state.subnetMask.fromString(FACTORY_AP_SUBNET_MASK);
        }
    }
}

void APSettingsService::forceAPMode()
{
    (void)startAccessPoint(ApLaunchMode::ManualApOnly);
}

bool APSettingsService::startAccessPoint(ApLaunchMode mode)
{
    ensureLoadedState();

    if (_apStarted && _launchMode == mode)
    {
        return true;
    }

    ESP_LOGI(SVK_TAG,
             "Starting software access point (mode=%s, ssid=%s, channel=%u, hidden=%d)",
             apLaunchModeName(mode),
             _state.ssid.c_str(),
             (unsigned)_state.channel,
             _state.ssidHidden ? 1 : 0);

    // Do not pre-stop an already running AP when only the launch mode changes.
    // Reconfiguring in place gives rescue/manual transitions a better chance of
    // keeping the existing SSID alive if the new mode fails to apply.

    if (mode == ApLaunchMode::ManualApOnly)
    {
        // Manual AP-only mode intentionally tears down STA state.
        MDNS.end();
        WiFi.disconnect(true);
        vTaskDelay(pdMS_TO_TICKS(100));
        WiFi.mode(WIFI_AP);
    }
    else
    {
        // Rescue AP overlays on top of STA to keep uplink recovery alive.
        if (WiFi.getMode() != WIFI_AP_STA)
        {
            WiFi.mode(WIFI_AP_STA);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    bool okConfig = WiFi.softAPConfig(_state.localIP, _state.gatewayIP, _state.subnetMask);
    if (!okConfig)
    {
        ESP_LOGE(SVK_TAG, "softAPConfig failed (ip=%s, gw=%s, mask=%s)",
                 _state.localIP.toString().c_str(),
                 _state.gatewayIP.toString().c_str(),
                 _state.subnetMask.toString().c_str());
    }

    const char *passwordArg = nullptr;
    if (!_state.password.isEmpty())
    {
        passwordArg = _state.password.c_str();
    }

    bool okAp = WiFi.softAP(_state.ssid.c_str(), passwordArg, _state.channel, _state.ssidHidden, _state.maxClients);

    if (!okAp)
    {
        ESP_LOGE(SVK_TAG,
                 "softAP failed (mode=%s, ssid_len=%u, channel=%u, hidden=%d, max_clients=%u)",
                 apLaunchModeName(mode),
                 (unsigned)_state.ssid.length(),
                 (unsigned)_state.channel,
                 _state.ssidHidden ? 1 : 0,
                 (unsigned)_state.maxClients);
        _apStarted = false;
        _launchMode = ApLaunchMode::None;
        return false;
    }

    _apStarted = true;
    _launchMode = mode;

    IPAddress apIp = WiFi.softAPIP();
    ESP_LOGI(SVK_TAG, "AP started successfully (mode=%s, IP=%s, channel=%u, wifi_mode=%d)",
             apLaunchModeName(mode),
             apIp.toString().c_str(),
             (unsigned)WiFi.channel(),
             (int)WiFi.getMode());

    stopDnsServer();

    const String dnsName = resolveApDnsHostname();
    if (!dnsName.isEmpty()) {
        void* dnsMemory = ps_malloc(sizeof(DNSServer));
        if (dnsMemory) {
            _dnsServer = new(dnsMemory) DNSServer();
            _dnsServerAllocatedInPsram = true;
            ESP_LOGD(SVK_TAG, "Allocated AP DNS server in PSRAM");
        } else {
            _dnsServer = new(std::nothrow) DNSServer();
            _dnsServerAllocatedInPsram = false;
            if (_dnsServer) {
                ESP_LOGW(SVK_TAG, "Allocated AP DNS server in DRAM (PSRAM failed)");
            }
        }

        if (_dnsServer) {
            if (_dnsServer->start(DNS_PORT, dnsName.c_str(), apIp)) {
                ESP_LOGI(SVK_TAG, "AP DNS alias active: %s -> %s",
                         dnsName.c_str(),
                         apIp.toString().c_str());
            } else {
                ESP_LOGE(SVK_TAG, "Failed to start AP DNS alias server for %s", dnsName.c_str());
                stopDnsServer();
            }
        } else {
            ESP_LOGE(SVK_TAG, "Failed to allocate AP DNS alias server");
        }
    } else {
        ESP_LOGW(SVK_TAG, "Skipping AP DNS alias: hostname unavailable");
    }

#if CONFIG_IDF_TARGET_ESP32C3
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // https://www.wemos.cc/en/latest/c3/c3_mini_1_0_0.html#about-wifi
#endif
    // Keep a narrow AP-local DNS alias for <hostname>.local so first-run access
    // remains stable even on clients that do not resolve mDNS reliably in AP mode.

    return true;
}

void APSettingsService::stopDnsServer()
{
    if (!_dnsServer)
    {
        return;
    }

    _dnsServer->stop();
    if (_dnsServerAllocatedInPsram)
    {
        _dnsServer->~DNSServer();
        free(_dnsServer);
    }
    else
    {
        delete _dnsServer;
    }

    _dnsServer = nullptr;
    _dnsServerAllocatedInPsram = false;
}

void APSettingsService::stopAP()
{
    if (_dnsServer)
    {
        ESP_LOGI(SVK_TAG, "Stopping AP DNS alias server");
        stopDnsServer();
    }

    ESP_LOGI(SVK_TAG, "Stopping software access point (mode=%s)", apLaunchModeName(_launchMode));
    WiFi.softAPdisconnect(true);
    if (_launchMode == ApLaunchMode::RescueApSta)
    {
        WiFi.mode(WIFI_STA);
    }
    _apStarted = false;
    _launchMode = ApLaunchMode::None;
}

void APSettingsService::stopAccessPoint()
{
    if (!_apStarted)
    {
        return;
    }
    stopAP();
}

void APSettingsService::handleDNS()
{
    if (_dnsServer)
    {
        _dnsServer->processNextRequest();
    }
}

APNetworkStatus APSettingsService::getAPNetworkStatus()
{
    // Diagnose the AP that this service actually started instead of inferring
    // activity from WiFi.mode(), which can stay in AP/AP+STA after a failed
    // SoftAP bring-up and make the dashboard report a phantom AP.
    return _apStarted ? APNetworkStatus::ACTIVE : APNetworkStatus::INACTIVE;
}
