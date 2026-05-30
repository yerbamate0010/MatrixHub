#pragma once

#include <Arduino.h>
#include <esp_sleep.h>

namespace POWER {

enum class WakeReason {
    Unknown,
    Timer,
    Button,
    Other
};

class PowerWakeController {
public:
    void begin();
    WakeReason getWakeReason();
    esp_sleep_wakeup_cause_t getWakeupCauseRaw();
    uint64_t getGpioWakeupMask();
    uint64_t getExt1WakeupMask();
    void configureWakeSources(uint32_t wakeIntervalMs);
    
    // Test hooks
    void setConfigureWakeSourcesCallback(void (*callback)());
    void resetTestHooks();

private:
    WakeReason _wakeReason = WakeReason::Unknown;
    esp_sleep_wakeup_cause_t _wakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;
    uint64_t _gpioWakeupMask = 0;
    uint64_t _ext1WakeupMask = 0;
    void (*_configureWakeSourcesCallback)() = nullptr;
};

} // namespace POWER
