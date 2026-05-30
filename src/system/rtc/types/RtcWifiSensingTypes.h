#pragma once

#include <Arduino.h>
#include "../RtcDefaultValues.h"

namespace RTC {

/**
 * WiFi Sensing configuration
 */
struct __attribute__((packed)) WifiSensingData {
    bool enabled = Defaults::WifiSensing::Enabled;
    uint16_t sampleIntervalMs = Defaults::WifiSensing::SampleIntervalMs;
    float varianceThreshold = Defaults::WifiSensing::VarianceThreshold;
};

} // namespace RTC
