/**
 * @file PowerSettings.cpp
 * @brief Power configuration - now backed by RTC memory
 * 
 * On cold boot: loads from NVS Preferences → RTC
 * On warm boot: uses RTC directly (no NVS access)
 * On change: writes to RTC + NVS (backup)
 */

#include "PowerSettings.h"
#include "../rtc/RtcConfig.h"
#include "../logging/Logging.h"

namespace POWER {

namespace {
    constexpr const char *kNamespace = "power_cfg";
    constexpr const char *kKeyInact = "inact_ms";
    constexpr const char *kKeyGrace = "grace_ms";
    constexpr const char *kKeySleepEnabled = "sleep_en";
}

void PowerSettings::begin() {
    // Data is already in RTC::store.power (loaded by RtcConfigLoader)
    // Open prefs for future writes only
    _prefsReady = _prefs.begin(kNamespace, false);
    if (!_prefsReady) {
        LOGE("prefs begin failed");
    }
}

bool PowerSettings::setInactivityTimeout(uint32_t timeoutMs) {
    // Write to RTC (transactional update with CRC recalc)
    const bool rtcUpdated = RTC::updateConfig([timeoutMs](RTC::ConfigStore& store) {
        store.power.inactivityTimeoutMs = timeoutMs;
    });
    if (!rtcUpdated) {
        LOGE("Failed to update RTC inactivity timeout");
        return false;
    }
    if (!_prefsReady) {
        LOGE("Preferences unavailable while saving inactivity timeout");
        return false;
    }
    // Backup to NVS
    const bool persisted = _prefs.putUInt(kKeyInact, timeoutMs) > 0;
    if (!persisted) {
        LOGE("Failed to persist inactivity timeout");
        return false;
    }
    LOGI("Saved inactivity=%u ms", timeoutMs);
    return true;
}

bool PowerSettings::setGracePeriod(uint32_t graceMs) {
    const bool rtcUpdated = RTC::updateConfig([graceMs](RTC::ConfigStore& store) {
        store.power.graceAfterBootMs = graceMs;
    });
    if (!rtcUpdated) {
        LOGE("Failed to update RTC grace period");
        return false;
    }
    if (!_prefsReady) {
        LOGE("Preferences unavailable while saving grace period");
        return false;
    }
    const bool persisted = _prefs.putUInt(kKeyGrace, graceMs) > 0;
    if (!persisted) {
        LOGE("Failed to persist grace period");
        return false;
    }
    LOGI("Saved grace=%u ms", graceMs);
    return true;
}

uint32_t PowerSettings::getInactivityTimeout() const {
    return RTC::getConfig().power.inactivityTimeoutMs;
}

uint32_t PowerSettings::getGracePeriod() const {
    return RTC::getConfig().power.graceAfterBootMs;
}

bool PowerSettings::getSleepEnabled() const {
    return RTC::getConfig().power.sleepEnabled;
}

bool PowerSettings::setSleepEnabled(bool enabled) {
    const bool rtcUpdated = RTC::updateConfig([enabled](RTC::ConfigStore& store) {
        store.power.sleepEnabled = enabled;
    });
    if (!rtcUpdated) {
        LOGE("Failed to update RTC sleep_enabled");
        return false;
    }
    if (!_prefsReady) {
        LOGE("Preferences unavailable while saving sleep_enabled");
        return false;
    }
    const bool persisted = _prefs.putBool(kKeySleepEnabled, enabled) > 0;
    if (!persisted) {
        LOGE("Failed to persist sleep_enabled");
        return false;
    }
    LOGI("Saved sleep_enabled=%s", enabled ? "true" : "false");
    return true;
}

InactivityConfig PowerSettings::getInactivityConfig() const {
    return InactivityConfig{
        RTC::getConfig().power.inactivityTimeoutMs,
        RTC::getConfig().power.graceAfterBootMs,
        RTC::getConfig().power.sleepEnabled
    };
}

} // namespace POWER
