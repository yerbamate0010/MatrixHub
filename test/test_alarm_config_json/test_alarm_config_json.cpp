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

// Include .cpp directly to avoid linker issues in native test environment without complex build flags
#include "../../src/config/json/AlarmConfigJson.cpp"
#include "../../src/alarms/types/AlarmRule.h"
#include "../../src/alarms/types/AlarmEnums.h"

// Stubs for RTC functions used by AlarmConfigJson but not by the functions under test
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

namespace ALARMS {
namespace RULES_CONFIG {
    static RTC::AlarmRulesData mockRules;

    bool copyTo(RTC::AlarmRulesData& out) {
        out = mockRules;
        return true;
    }
    void withRules(const std::function<void(const RTC::AlarmRulesData&)>& reader) { reader(mockRules); }
    bool update(const std::function<void(RTC::AlarmRulesData&)>& updater) {
        updater(mockRules);
        return true;
    }
}
}

void test_deserialize_source_string_wifi_motion() {
    JsonDocument doc;
    doc["id"] = "test";
    doc["name"] = "test";
    doc["source"] = "wifi_motion";
    
    JsonObject obj = doc.as<JsonObject>();
    ALARMS::AlarmRule rule;
    
    CONFIG::JSON::deserializeAlarmRule(obj, rule);
    
    TEST_ASSERT_EQUAL(ALARMS::AlarmSource::WifiMotion, rule.source);
}

void test_deserialize_source_int_wifi_motion() {
    JsonDocument doc;
    doc["id"] = "test";
    doc["name"] = "test";
    doc["source"] = 3; // WifiMotion enum value
    
    JsonObject obj = doc.as<JsonObject>();
    ALARMS::AlarmRule rule;
    
    CONFIG::JSON::deserializeAlarmRule(obj, rule);
    
    TEST_ASSERT_EQUAL(ALARMS::AlarmSource::WifiMotion, rule.source);
}

void test_deserialize_operator_string_below() {
    JsonDocument doc;
    doc["id"] = "test";
    doc["name"] = "test";
    doc["operator"] = "below";
    
    JsonObject obj = doc.as<JsonObject>();
    ALARMS::AlarmRule rule;
    
    CONFIG::JSON::deserializeAlarmRule(obj, rule);
    
    TEST_ASSERT_EQUAL(ALARMS::AlarmOperator::Below, rule.op);
}

void test_deserialize_severity_string_critical() {
    JsonDocument doc;
    doc["id"] = "test";
    doc["name"] = "test";
    doc["severity"] = "critical";
    
    JsonObject obj = doc.as<JsonObject>();
    ALARMS::AlarmRule rule;
    
    CONFIG::JSON::deserializeAlarmRule(obj, rule);
    
    TEST_ASSERT_EQUAL(ALARMS::AlarmSeverity::Critical, rule.severity);
}

void test_deserialize_notify_channels_unknown_string_fails() {
    JsonDocument doc;
    doc["id"] = "test";
    doc["name"] = "test";
    JsonArray channels = doc["notify_channels"].to<JsonArray>();
    channels.add("telegram");
    channels.add("unknown");

    JsonObject obj = doc.as<JsonObject>();
    ALARMS::AlarmRule rule;

    TEST_ASSERT_FALSE(CONFIG::JSON::deserializeAlarmRule(obj, rule));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_deserialize_source_string_wifi_motion);
    RUN_TEST(test_deserialize_source_int_wifi_motion);
    RUN_TEST(test_deserialize_operator_string_below);
    RUN_TEST(test_deserialize_severity_string_critical);
    RUN_TEST(test_deserialize_notify_channels_unknown_string_fails);
    return UNITY_END();
}
