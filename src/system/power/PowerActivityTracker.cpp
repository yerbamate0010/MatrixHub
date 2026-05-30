/**
 * @file PowerActivityTracker.cpp
 * @brief Implementation of activity tracking coordinator (facade)
 */

#include "PowerActivityTracker.h"
#include "PowerSettings.h"
#include "PowerSleepController.h"
#include "../logging/Logging.h"
#include <WiFi.h>

namespace POWER {



uint32_t PowerActivityTracker::nowMs() {
    if (_timeProvider) {
        return _timeProvider();
    }
    return millis();
}

void PowerActivityTracker::begin() {
    uint32_t now = nowMs();
    uint32_t lastActivityMs = 0;
    uint32_t bootMs = 0;
    
    // Try to restore from RTC memory
    if (!ActivityPersistenceManager::tryRestore(lastActivityMs, bootMs)) {
        // Cold boot or corruption - initialize fresh
        bootMs = now;
        lastActivityMs = now;
        LOGI("[Power] Activity tracking initialized (cold boot)");
    } else {
        // Restored from RTC - adjust for new millis() base
        // Note: millis() resets to 0 after reboot, but RTC data persists
        // We treat restored activity as if it just happened (reset to boot time)
        bootMs = now;
        lastActivityMs = now;
        LOGI("[Power] Activity tracking resumed after reboot");
    }
    
    // Initialize modules
    ActivityMonitor::begin(bootMs, lastActivityMs);
    ActivityLogger::begin();
    
    // Save initial state to RTC
    ActivityPersistenceManager::save(lastActivityMs, bootMs);
}

uint32_t PowerActivityTracker::getLastActivityMs() {
    return ActivityMonitor::getLastActivityMs();
}

uint32_t PowerActivityTracker::getBootMs() {
    return ActivityMonitor::getBootMs();
}

void PowerActivityTracker::notifyActivity(const char *source) {
    uint32_t now = nowMs();
    ActivityMonitor::notifyActivity(now);
    ActivityLogger::logActivity(source, now);
}

void PowerActivityTracker::loopTick(PowerSleepController& sleepController, const PowerSettings& settings) {
    // If sleep is already requested, we don't need to check for inactivity
    if (sleepController.isSleepRequested()) {
        return;
    }

    uint32_t now = nowMs();

    // Treat AP station presence as activity to avoid sleeping while user connects
    int apStations = _apStationsProvider ? _apStationsProvider() : WiFi.softAPgetStationNum();
    if (apStations > 0) {
        ActivityLogger::logApClient(apStations);
        // AP presence is a sustained state, not a discrete user action.
        // Refresh the inactivity timer without emitting per-tick activity logs.
        ActivityMonitor::notifyActivity(now);
        return;  // do not sleep while AP clients are connected
    } else {
        ActivityLogger::resetApClient();
    }

    uint32_t gracePeriod = settings.getGracePeriod();
    if (ActivityMonitor::isInGracePeriod(now, gracePeriod)) {
        uint32_t remainingMs = gracePeriod - (now - ActivityMonitor::getBootMs());
        ActivityLogger::logGracePeriod(remainingMs);
        return;
    }

    uint32_t timeoutMs = settings.getInactivityTimeout();
    if (timeoutMs == 0) {
        return;
    }

    // Check if auto-sleep is enabled
    if (!settings.getSleepEnabled()) {
        return;
    }

    if (ActivityMonitor::isInactive(now, timeoutMs)) {
        uint32_t idleMs = ActivityMonitor::getIdleMs(now);
        LOGI("[Power] Inactivity timeout -> sleep (idle=%lus tmo=%lus)",
             static_cast<unsigned long>(idleMs / 1000UL),
             static_cast<unsigned long>(timeoutMs / 1000UL));
        sleepController.requestSleep("inactivity");
        return;
    }

    // Periodic countdown log
    uint32_t remainingMs = ActivityMonitor::getRemainingMs(now, timeoutMs);
    uint32_t idleMs = ActivityMonitor::getIdleMs(now);
    ActivityLogger::logCountdown(now, remainingMs / 1000UL, idleMs / 1000UL, timeoutMs / 1000UL);
}

void PowerActivityTracker::setTimeProvider(uint32_t (*provider)()) {
    _timeProvider = provider;
}

void PowerActivityTracker::setApStationsProvider(int (*provider)()) {
    _apStationsProvider = provider;
}

void PowerActivityTracker::resetTestHooks() {
    _timeProvider = nullptr;
    _apStationsProvider = nullptr;
}

} // namespace POWER
