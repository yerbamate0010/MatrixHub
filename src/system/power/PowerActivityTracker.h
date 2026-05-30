/**
 * @file PowerActivityTracker.h
 * @brief Activity tracking coordinator (facade)
 * 
 * Coordinates activity tracking through specialized modules:
 * - ActivityPersistenceManager - RTC memory save/restore
 * - ActivityMonitor - Activity tracking and inactivity detection
 * - ActivityLogger - Rate-limited logging
 */

#pragma once

#include <Arduino.h>
#include "activity/ActivityPersistence.h"
#include "activity/ActivityMonitor.h"
#include "activity/ActivityLogger.h"

namespace POWER {

class PowerSleepController;
class PowerSettings;

class PowerActivityTracker {
public:
    void begin();
    void loopTick(PowerSleepController& sleepController, const PowerSettings& settings);
    void notifyActivity(const char *source = nullptr);
    
    uint32_t getLastActivityMs();
    uint32_t getBootMs();
    uint32_t nowMs();

    // Test hooks
    void setTimeProvider(uint32_t (*provider)());
    void setApStationsProvider(int (*provider)());
    void resetTestHooks();

private:
    uint32_t (*_timeProvider)() = nullptr;
    int (*_apStationsProvider)() = nullptr;
};

} // namespace POWER
