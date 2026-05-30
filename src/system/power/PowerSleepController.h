#pragma once

#include <Arduino.h>

namespace POWER {

class PowerWakeController;

class PowerSleepController {
public:
    void begin(PowerWakeController* wakeController);
    void loopTick();
    
    void requestSleep(const char *reason, uint32_t delayMs = 0);
    bool isSleepRequested();
    uint32_t getSleepEtaMs();
    const char* getSleepReason();

    
    void setWakeInterval(uint32_t intervalMs);
    uint32_t getWakeInterval();

    void setPreSleepHook(void (*hook)());
    
    // Test hooks
    void setSleepCallback(void (*callback)(const char*));
    void resetTestHooks();

private:
    PowerWakeController* _wakeController = nullptr;
    void enterDeepSleep(const char *reason);

    bool _sleepRequested = false;
    uint32_t _sleepRequestAtMs = 0;
    uint32_t _sleepDelayMs = 0;
    const char *_sleepReason = nullptr;
    uint32_t _wakeIntervalMs = 0;
    
    void (*_preSleepHook)() = nullptr;
    void (*_sleepCallback)(const char *reason) = nullptr;
    
    bool _isEnteringDeepSleep = false;
};

} // namespace POWER
