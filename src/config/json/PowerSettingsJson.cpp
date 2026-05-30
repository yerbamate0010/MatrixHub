#include "PowerSettingsJson.h"
#include "../../config/App.h"
#include "../../system/rtc/RtcConfig.h"

namespace CONFIG {
namespace JSON {

// ---------------------------------------------------------------------------
// deserializePower  —  shared by loadPower (file) AND API endpoint.
// Updates only fields present in JSON (partial-update safe).
// ---------------------------------------------------------------------------
void deserializePower(JsonObject& obj, RTC::PowerData& data) {
    if (obj[Keys::kSleepEnabled].is<bool>()) {
        data.sleepEnabled = obj[Keys::kSleepEnabled].as<bool>();
    }
    if (obj[Keys::kInactivityTimeoutMs].is<uint32_t>()) {
        uint32_t v = obj[Keys::kInactivityTimeoutMs].as<uint32_t>();
        v = std::max(POWER::INACTIVITY_TIMEOUT_MIN_MS, std::min(v, POWER::INACTIVITY_TIMEOUT_MAX_MS));
        data.inactivityTimeoutMs = v;
    }
    if (obj[Keys::kGraceAfterBootMs].is<uint32_t>()) {
        uint32_t v = obj[Keys::kGraceAfterBootMs].as<uint32_t>();
        v = std::max(POWER::GRACE_MIN_MS, std::min(v, POWER::GRACE_MAX_MS));
        data.graceAfterBootMs = v;
    }
}

void loadPower(JsonObject& obj) {
    RTC::updateConfigSection(&RTC::ConfigStore::power, [&](RTC::PowerData& power) {
        power = RTC::PowerData{};
        deserializePower(obj, power);
    });
}

void savePower(JsonObject& obj) {
    RTC::PowerData p{};
    RTC::withConfig([&](const RTC::ConfigStore& cfg) {
        p = cfg.power;
    });
    obj[Keys::kInactivityTimeoutMs] = p.inactivityTimeoutMs;
    obj[Keys::kGraceAfterBootMs] = p.graceAfterBootMs;
    obj[Keys::kSleepEnabled] = p.sleepEnabled;
}

} // namespace JSON
} // namespace CONFIG
