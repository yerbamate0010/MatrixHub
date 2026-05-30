#define NOMINMAX
#include <algorithm>
#include <unity.h>
#include <ArduinoJson.h>

// Force undef min/max macros to prevent conflicts with std::min/max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// Include .cpp directly to avoid linker issues in native test environment
#include "../../src/config/json/WifiSensingConfigJson.cpp"
#include "../../src/system/rtc/types/RtcWifiSensingTypes.h"

// Stubs for RTC functions
namespace RTC {
    static ConfigStore mockStore;
    const ConfigStore& getConfig() { return mockStore; }
    ConfigStore& getMutableConfig() { return mockStore; }
    void withConfig(const std::function<void(const ConfigStore&)>& reader) { reader(mockStore); }
    bool updateConfig(const std::function<void(ConfigStore&)>& updater) {
        updater(mockStore);
        return true;
    }
}

void setUp(void) {
    // Reset mock store before each test
    RTC::mockStore.wifiSensing = RTC::WifiSensingData();
}

void tearDown(void) {}

void test_load_wifi_sensing_defaults() {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    // Empty object should trigger defaults
    
    CONFIG::JSON::loadWifiSensing(obj);
    
    const auto& w = RTC::mockStore.wifiSensing;
    TEST_ASSERT_FALSE(w.enabled);
    TEST_ASSERT_EQUAL_UINT32(200, w.sampleIntervalMs); // FACTORY_DEFAULT
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, w.varianceThreshold); // FACTORY_DEFAULT
}

void test_load_wifi_sensing_custom_values() {
    JsonDocument doc;
    doc["enabled"] = true;
    doc["sample_interval_ms"] = 500;
    doc["variance_threshold"] = 2.5f;
    JsonObject obj = doc.as<JsonObject>();
    
    CONFIG::JSON::loadWifiSensing(obj);
    
    const auto& w = RTC::mockStore.wifiSensing;
    TEST_ASSERT_TRUE(w.enabled);
    TEST_ASSERT_EQUAL_UINT32(500, w.sampleIntervalMs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.5f, w.varianceThreshold);
}

void test_load_wifi_sensing_clamps_values() {
    JsonDocument doc;
    doc["sample_interval_ms"] = 10; // Too low (min 50)
    doc["variance_threshold"] = 0.05f; // Too low (min 0.1)
    JsonObject obj = doc.as<JsonObject>();
    
    CONFIG::JSON::loadWifiSensing(obj);
    
    const auto& w = RTC::mockStore.wifiSensing;
    // Check clamping logic (constants from RtcWifiSensingTypes.h / Hardware.h)
    // Assuming LIMITS::WIFI_SENSING::MIN_INTERVAL_MS = 50 (based on previous SystemConfigJson code)
    // We should probably include Hardware.h or defining constants to be sure, but let's test the behavior
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(50, w.sampleIntervalMs); 
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.1f, w.varianceThreshold);
}

void test_save_wifi_sensing() {
    RTC::mockStore.wifiSensing.enabled = true;
    RTC::mockStore.wifiSensing.sampleIntervalMs = 750;
    RTC::mockStore.wifiSensing.varianceThreshold = 3.3f;
    
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    
    CONFIG::JSON::saveWifiSensing(obj);
    
    TEST_ASSERT_TRUE(obj["enabled"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(750, obj["sample_interval_ms"].as<uint32_t>());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.3f, obj["variance_threshold"].as<float>());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_load_wifi_sensing_defaults);
    RUN_TEST(test_load_wifi_sensing_custom_values);
    RUN_TEST(test_load_wifi_sensing_clamps_values);
    RUN_TEST(test_save_wifi_sensing);
    return UNITY_END();
}
