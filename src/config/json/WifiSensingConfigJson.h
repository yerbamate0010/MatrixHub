#pragma once

#include <ArduinoJson.h>
#include "../../system/rtc/types/RtcWifiSensingTypes.h"

namespace CONFIG {
namespace JSON {

    void deserializeWifiSensing(JsonObject& obj, RTC::WifiSensingData& data);
    void loadWifiSensing(JsonObject& obj);
    void saveWifiSensing(JsonObject& obj);

} // namespace JSON
} // namespace CONFIG
