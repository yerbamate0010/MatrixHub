#include "WifiSensingConfigJson.h"
#include "../App.h"
#include "../../system/rtc/RtcConfig.h"
#include <algorithm>

namespace CONFIG {
namespace JSON {

void deserializeWifiSensing(JsonObject& obj, RTC::WifiSensingData& data) {
    if (obj[Keys::kEnabled].is<bool>()) {
        bool v = obj[Keys::kEnabled].as<bool>();
        data.enabled = v;
    }
    
    if (obj[Keys::kSampleIntervalMs].is<uint32_t>()) {
        uint32_t interval = obj[Keys::kSampleIntervalMs].as<uint32_t>();
        data.sampleIntervalMs = (std::max)((uint32_t)LIMITS::WIFI_SENSING::MIN_INTERVAL_MS, 
                                    (std::min)(interval, (uint32_t)LIMITS::WIFI_SENSING::MAX_INTERVAL_MS));
    }
                                
    if (obj[Keys::kVarianceThreshold].is<float>()) {
        float variance = obj[Keys::kVarianceThreshold].as<float>();
        data.varianceThreshold = (std::max)(LIMITS::WIFI_SENSING::MIN_VARIANCE, 
                                     (std::min)(variance, LIMITS::WIFI_SENSING::MAX_VARIANCE));
    }
}

void loadWifiSensing(JsonObject& obj) {
    RTC::updateConfigSection(&RTC::ConfigStore::wifiSensing, [&](RTC::WifiSensingData& wifiSensing) {
        deserializeWifiSensing(obj, wifiSensing);
    });
}

void saveWifiSensing(JsonObject& obj) {
    RTC::WifiSensingData w{};
    RTC::withConfig([&](const RTC::ConfigStore& cfg) {
        w = cfg.wifiSensing;
    });
    obj[Keys::kEnabled] = w.enabled;
    obj[Keys::kSampleIntervalMs] = w.sampleIntervalMs;
    obj[Keys::kVarianceThreshold] = w.varianceThreshold;
}

} // namespace JSON
} // namespace CONFIG
