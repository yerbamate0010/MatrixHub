#include <unity.h>
#include <ArduinoJson.h>
#include <esp_log.h>

#include "../../lib/framework/utils/SettingValue.cpp"
#include "../../lib/framework/wifi/WiFiSettingsService.h"

void setUp(void) {}
void tearDown(void) {}

void test_update_accepts_32_byte_ssid_and_preserves_static_ip_config() {
    WiFiSettings settings;

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["hostname"] = "matrixhub";
    root["connection_mode"] = 1;

    JsonObject wifi = root["wifi_networks"].to<JsonArray>().add<JsonObject>();
    wifi["ssid"] = "12345678901234567890123456789012";
    wifi["password"] = "secret123";
    wifi["static_ip_config"] = true;
    wifi["local_ip"] = "192.168.1.50";
    wifi["gateway_ip"] = "192.168.1.1";
    wifi["subnet_mask"] = "255.255.255.0";
    wifi["dns_ip_1"] = "8.8.8.8";

    const StateUpdateResult result = WiFiSettings::update(root, settings, "http");

    TEST_ASSERT_EQUAL(static_cast<int>(StateUpdateResult::CHANGED), static_cast<int>(result));
    TEST_ASSERT_EQUAL_STRING("matrixhub", settings.hostname.c_str());
    TEST_ASSERT_EQUAL(1, settings.wifiSettings.size());

    const wifi_settings_t& network = settings.wifiSettings.front();
    TEST_ASSERT_EQUAL_STRING("12345678901234567890123456789012", network.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("secret123", network.password.c_str());
    TEST_ASSERT_TRUE(network.staticIPConfig);
    TEST_ASSERT_TRUE(network.localIP == IPAddress(192, 168, 1, 50));
    TEST_ASSERT_TRUE(network.gatewayIP == IPAddress(192, 168, 1, 1));
    TEST_ASSERT_TRUE(network.subnetMask == IPAddress(255, 255, 255, 0));
    TEST_ASSERT_TRUE(network.dnsIP1 == IPAddress(8, 8, 8, 8));
    TEST_ASSERT_TRUE(network.dnsIP2 == IPAddress(INADDR_NONE));
}

void test_update_rejects_33_byte_ssid() {
    WiFiSettings settings;

    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["hostname"] = "matrixhub";
    root["connection_mode"] = 1;

    JsonObject wifi = root["wifi_networks"].to<JsonArray>().add<JsonObject>();
    wifi["ssid"] = "123456789012345678901234567890123";
    wifi["password"] = "secret123";

    const StateUpdateResult result = WiFiSettings::update(root, settings, "http");

    TEST_ASSERT_EQUAL(static_cast<int>(StateUpdateResult::CHANGED), static_cast<int>(result));
    TEST_ASSERT_EQUAL_STRING("matrixhub", settings.hostname.c_str());
    TEST_ASSERT_EQUAL(0, settings.wifiSettings.size());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_update_accepts_32_byte_ssid_and_preserves_static_ip_config);
    RUN_TEST(test_update_rejects_33_byte_ssid);
    return UNITY_END();
}
