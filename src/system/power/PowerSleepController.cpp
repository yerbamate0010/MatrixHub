#include "PowerSleepController.h"
#include "PowerWakeController.h"
#include "../logging/Logging.h"
#include "../rtc/RtcConfig.h"
#include "../../config/App.h"

#ifndef NATIVE_BUILD
#include <services/SleepService.h>
#endif

#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace POWER {

void PowerSleepController::begin(PowerWakeController* wakeController) {
    _wakeController = wakeController;
    _sleepRequested = false;
    _sleepRequestAtMs = 0;
    _sleepDelayMs = 0;
    _isEnteringDeepSleep = false;
    _wakeIntervalMs = POWER::WAKE_INTERVAL_MS;
}

void PowerSleepController::setWakeInterval(uint32_t intervalMs) {
    _wakeIntervalMs = intervalMs;
}

uint32_t PowerSleepController::getWakeInterval() {
    return _wakeIntervalMs;
}

void PowerSleepController::requestSleep(const char *reason, uint32_t delayMs) {
    // Force minimum delay to avoid executing shutdown synchronously
    // inside caller's context (e.g. HTTP handler → MDNS.end() deadlock).
    static constexpr uint32_t MIN_SLEEP_DELAY_MS = 50;
    if (delayMs < MIN_SLEEP_DELAY_MS) {
        delayMs = MIN_SLEEP_DELAY_MS;
    }

    _sleepRequested = true;
    _sleepDelayMs = delayMs;
    _sleepRequestAtMs = millis();
    _sleepReason = reason;
    LOGI("[Power] Sleep requested (%s, +%lu ms)", reason ? reason : "unknown", static_cast<unsigned long>(delayMs));
}

bool PowerSleepController::isSleepRequested() {
    return _sleepRequested;
}

uint32_t PowerSleepController::getSleepEtaMs() {
    if (!_sleepRequested) {
        return 0;
    }
    uint32_t now = millis();
    uint32_t elapsed = now - _sleepRequestAtMs;
    if (elapsed >= _sleepDelayMs) {
        return 0;
    }
    return _sleepDelayMs - elapsed;
}

void PowerSleepController::loopTick() {
    if (_sleepRequested) {
        uint32_t now = millis();
        if (now - _sleepRequestAtMs >= _sleepDelayMs) {
            enterDeepSleep(_sleepReason ? _sleepReason : "pending");
        }
    }
}

void PowerSleepController::enterDeepSleep(const char *reason) {
    if (_sleepCallback) {
        _sleepCallback(reason);
        return;
    }

    if (_isEnteringDeepSleep) {
        LOGW("[Power] enterDeepSleep() called twice; ignoring (reason=%s)", reason ? reason : "unknown");
        return;
    }
    _isEnteringDeepSleep = true;
    _sleepRequested = true;
    _sleepDelayMs = 0;

        LOGI("[Power] Entering deep sleep (reason: %s). Wake timer: %.1fs.",
            reason ? reason : "unknown",
            static_cast<float>(_wakeIntervalMs) / 1000.0f);

    // Call framework sleep callbacks
    SleepService::executeSleepCallbacks();
    LOGI("[Power] Sleep callbacks executed");

    // Call application pre-sleep hook
    if (_preSleepHook) {
        _preSleepHook();
    }

    if (_wakeController) {
        _wakeController->configureWakeSources(_wakeIntervalMs);
    }

#ifdef ESP_PD_DOMAIN_RTC_SLOW_MEM
    // Explicitly keep RTC Slow Memory powered (where RTC_DATA_ATTR lives)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
#endif
#ifdef ESP_PD_DOMAIN_RTC_FAST_MEM
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
#endif

    // Final RTC snapshot just before sleep entry (after pre-sleep hook and task stop).
    RTC::prepareForSleep();

    LOGI("[Power] Calling esp_deep_sleep_start() now");
    vTaskDelay(pdMS_TO_TICKS(100)); // Give UART and FS a moment to flush (RTOS-safe)
    esp_deep_sleep_start();
}

const char* PowerSleepController::getSleepReason() {
    return _sleepReason;
}

void PowerSleepController::setPreSleepHook(void (*hook)()) {
    _preSleepHook = hook;
}

void PowerSleepController::setSleepCallback(void (*callback)(const char *)) {
    _sleepCallback = callback;
}

void PowerSleepController::resetTestHooks() {
    _sleepCallback = nullptr;
}

} // namespace POWER
