/**
 * @file test_config_system.cpp
 * @brief Unit tests for System configuration loading resilience
 */

#include <unity.h>
#include <ArduinoJson.h>

#include "../../src/system/rtc/types/RtcSystemTypes.h"
#include "../../src/system/logging/Logging.h"

// Fix Arduino macro collision with std::min/max
#undef min
#undef max

#include "../../src/system/rtc/RtcConfig.h"

// Provide mock Logging definitions
namespace LOG {
    Settings Logging::_settings;
    void Logging::log(esp_log_level_t level, const char *tag, const char *fmt, ...) {}
    void Logging::logSection(const char* title) {}
    void Logging::logStackHwm(const char* name, uint32_t period) {}
    void Logging::logStackHwm(const char* name, uint32_t period, uint32_t minFreeBytes) {}
    
    const char* Logging::levelToString(esp_log_level_t level) { return "info"; }
    esp_log_level_t Logging::stringToLevel(std::string_view levelStr, esp_log_level_t defaultLevel) { return ESP_LOG_DEBUG; }
}

namespace RTC {
    static RTC::ConfigStore mockConfig;
    const RTC::ConfigStore& getConfig() { return mockConfig; }
    RTC::ConfigStore& getMutableConfig() { return mockConfig; }
    void withConfig(const std::function<void(const ConfigStore&)>& reader) { reader(mockConfig); }
    bool updateConfig(const std::function<void(ConfigStore&)>& updater) {
        updater(mockConfig);
        return true;
    }
}

namespace SYSTEM {
namespace HEARTBEAT_CONFIG {
    static RTC::HeartbeatData mockHeartbeat;

    RTC::HeartbeatData copy() { return mockHeartbeat; }
    void withConfig(const std::function<void(const RTC::HeartbeatData&)>& reader) { reader(mockHeartbeat); }
    bool update(const std::function<void(RTC::HeartbeatData&)>& updater) {
        updater(mockHeartbeat);
        return true;
    }
}
}

#include "../../src/config/json/SystemConfigJson.cpp"
#include "../../src/system/health/heartbeat/HeartbeatConfigSanitizer.cpp"

using namespace CONFIG::JSON;

void setUp(void) {}
void tearDown(void) {}

void test_config_empty_object_partial_update() {
    // Empty JSON document
    JsonDocument doc;
    deserializeJson(doc, "{}");
    JsonObject obj = doc.as<JsonObject>();

    RTC::HeartbeatData heartbeat;
    // Current value
    heartbeat.intervalMs = 777;

    deserializeHeartbeat(obj, heartbeat);
    
    // Heartbeat config is sanitized after deserialize, so an invalid current value
    // is normalized back to the configured default.
    TEST_ASSERT_EQUAL(HEARTBEAT::DEFAULT_INTERVAL_MS, heartbeat.intervalMs);
}

void test_config_corrupted_json_bootloop_safety() {
    // Malformed JSON (truncated mid-object)
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, "{\"intervalMs\": 100");
    
    TEST_ASSERT_EQUAL(DeserializationError::IncompleteInput, err.code());
    
    JsonObject obj = doc.as<JsonObject>();
    RTC::LoggingData logging;
    logging.level = ESP_LOG_DEBUG;
    
    deserializeLogging(obj, logging);
    
    // Safety check - should not crash, and should not modify default data
    TEST_ASSERT_EQUAL(ESP_LOG_DEBUG, logging.level);
}

void test_config_type_mismatch_safety() {
    // String instead of number
    JsonDocument doc;
    deserializeJson(doc, "{\"intervalMs\": \"fast\", \"port\": \"abcd\"}");
    JsonObject obj = doc.as<JsonObject>();

    RTC::UdpPusherData udp;
    udp.intervalMs = 5000;
    udp.port = 1234;

    deserializeUdpPusher(obj, udp);
    
    // Type mismatches in ArduinoJson with .is<T>() check result in no change
    TEST_ASSERT_EQUAL(1234, udp.port); 
    TEST_ASSERT_EQUAL(5000, udp.intervalMs);
}

void test_task_stack_budgets_track_feature_stack_sizes() {
    TEST_ASSERT_EQUAL_UINT32(
        CONFIG::TASKS::STACK_SENSOR_LOGGING,
        CONFIG::TASKS::STACK_BUDGET_SENSOR_LOGGING.stackBytes);
    TEST_ASSERT_EQUAL_UINT32(
        CONFIG::TASKS::STACK_NOTIFICATION_WORKER,
        CONFIG::TASKS::STACK_BUDGET_NOTIFICATION_WORKER.stackBytes);
    TEST_ASSERT_EQUAL_UINT32(
        CONFIG::TASKS::STACK_HEARTBEAT,
        CONFIG::TASKS::STACK_BUDGET_HEARTBEAT.stackBytes);
    TEST_ASSERT_EQUAL_UINT32(
        CONFIG::TASKS::STACK_WIFI_SENSING_CSI,
        CONFIG::TASKS::STACK_BUDGET_WIFI_SENSING_CSI.stackBytes);

    TEST_ASSERT_GREATER_THAN_UINT32(0, CONFIG::TASKS::STACK_BUDGET_SENSOR_LOGGING.minFreeBytes);
    TEST_ASSERT_GREATER_THAN_UINT32(0, CONFIG::TASKS::STACK_BUDGET_NOTIFICATION_WORKER.minFreeBytes);
    TEST_ASSERT_GREATER_THAN_UINT32(0, CONFIG::TASKS::STACK_BUDGET_HEARTBEAT.minFreeBytes);
    TEST_ASSERT_GREATER_THAN_UINT32(0, CONFIG::TASKS::STACK_BUDGET_WIFI_SENSING_CSI.minFreeBytes);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_config_empty_object_partial_update);
    RUN_TEST(test_config_corrupted_json_bootloop_safety);
    RUN_TEST(test_config_type_mismatch_safety);
    RUN_TEST(test_task_stack_budgets_track_feature_stack_sizes);
    return UNITY_END();
}

#if defined(ARDUINO) || defined(ESP_PLATFORM)
void setup() { delay(2000); main(0, nullptr); }
void loop() {}
#endif
