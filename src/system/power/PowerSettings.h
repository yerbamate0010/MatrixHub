/**
 * @file Hardware.h
 * @brief Power configuration interface - backed by RTC memory
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "../../config/App.h"

namespace POWER {

struct InactivityConfig {
    uint32_t timeoutMs;
    uint32_t graceAfterBootMs;
    bool sleepEnabled;
};

class PowerSettings {
public:
    void begin();
    bool setInactivityTimeout(uint32_t timeoutMs);
    bool setGracePeriod(uint32_t graceMs);
    bool setSleepEnabled(bool enabled);
    uint32_t getInactivityTimeout() const;
    uint32_t getGracePeriod() const;
    bool getSleepEnabled() const;
    InactivityConfig getInactivityConfig() const;

private:
    Preferences _prefs;
    bool _prefsReady = false;
    // Note: _cfg removed - now using RTC::store.power directly
};

} // namespace POWER
