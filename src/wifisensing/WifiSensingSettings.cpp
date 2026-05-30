/**
 * @file WifiSensingSettings.cpp
 * @brief WiFi Sensing settings service - backed by RTC memory
 * 
 * Refactored 3 Jan 2026: Uses centralized ConfigManager for persistence.
 */

#include "WifiSensingSettings.h"
#include "WifiSensingService.h"
#include "../config/json/WifiSensingConfigJson.h"
#include "../system/logging/Logging.h"

#include <cstring>

#undef LOG_TAG
#define LOG_TAG "WifiSenseSettings"

namespace WIFISENSING {

WifiSensingSettings::WifiSensingSettings(FS* fs, WIFISENSING::WifiSensingService* service)
    : RtcStatefulService(&RTC::ConfigStore::wifiSensing),
      _fs(fs),
      _service(service) {
    
    addUpdateHandler([this](std::string_view originId) { 
        (void)originId;
        return onConfigUpdated();
    }, false);
}

void WifiSensingSettings::begin() {
    _lastPersistedState = _state;
    
    // Data is already in RTC
    LOGI("Settings (RTC): enabled=%d, interval=%ums, threshold=%.2f",
         _state.enabled ? 1 : 0, _state.sampleIntervalMs, _state.varianceThreshold);
    
    // Start service if enabled
    if (_state.enabled && _service) {
        if (!_service->begin(_state.sampleIntervalMs, _state.varianceThreshold)) {
            LOGE("Failed to apply persisted WiFi sensing state during boot");
        }
    }
}

void WifiSensingSettings::readState(RTC::WifiSensingData& settings, JsonObject& root) {
    root[CONFIG::Keys::kEnabled] = settings.enabled;
    root[CONFIG::Keys::kSampleIntervalMs] = settings.sampleIntervalMs;
    root[CONFIG::Keys::kVarianceThreshold] = settings.varianceThreshold;
}

StateUpdateResult WifiSensingSettings::updateState(
    JsonObject& jsonObject,
    RTC::WifiSensingData& settings,
    std::string_view originId) {
    (void)originId;
    RTC::WifiSensingData nextState = settings;
    CONFIG::JSON::deserializeWifiSensing(jsonObject, nextState);

    if (memcmp(&settings, &nextState, sizeof(RTC::WifiSensingData)) == 0) {
        return StateUpdateResult::UNCHANGED;
    }

    settings = nextState;
    return StateUpdateResult::CHANGED;
}

StateHandlerResult WifiSensingSettings::onConfigUpdated() {
    const RTC::WifiSensingData previousState = _lastPersistedState;
    const RTC::WifiSensingData nextState = _state;

    const auto applyRuntimeState = [this](const RTC::WifiSensingData& fromState,
                                          const RTC::WifiSensingData& toState) -> bool {
        if (!_service) {
            return true;
        }

        if (fromState.enabled != toState.enabled) {
            if (toState.enabled) {
                LOGI("Enabling WiFi Sensing...");
                return _service->begin(toState.sampleIntervalMs, toState.varianceThreshold);
            }

            LOGI("Disabling WiFi Sensing...");
            return _service->stop();
        }

        if (!toState.enabled) {
            return true;
        }

        // Reconfigure the live worker in place. If runtime drift already left
        // the service stopped, start it fresh instead of reporting a false
        // "saved" success while sensing stays dead until reboot.
        LOGI("Updating WiFi Sensing settings...");
        if (!_service->isRunning()) {
            return _service->begin(toState.sampleIntervalMs, toState.varianceThreshold);
        }

        return _service->stop() &&
               _service->begin(toState.sampleIntervalMs, toState.varianceThreshold);
    };

    LOGI("Settings updated: enabled=%d, interval=%ums, threshold=%.2f",
         nextState.enabled ? 1 : 0, nextState.sampleIntervalMs, nextState.varianceThreshold);

    // Apply runtime first. If that fails, the outer transactional RTC service
    // can still roll the in-memory/RTC state back without leaving the live
    // worker silently diverged from the requested config.
    if (!applyRuntimeState(previousState, nextState)) {
        LOGE("Failed to apply WiFi sensing runtime state");
        return StateHandlerResult::failure("config/apply_failed");
    }

    // Persist only after runtime accepted the transition. If the filesystem
    // save fails, revert the live runtime back to the last known persisted
    // state so RTC rollback and runtime behavior stay aligned.
    if (!_fs || !CONFIG::save(*_fs)) {
        LOGE("Failed to persist WiFi sensing settings");
        if (!applyRuntimeState(nextState, previousState)) {
            LOGE("Failed to roll back WiFi sensing runtime after save failure");
        }
        return StateHandlerResult::failure("config/save_failed");
    }

    _lastPersistedState = nextState;
    return StateHandlerResult::success();
}

bool WifiSensingSettings::isEnabled() const {
    return _state.enabled;
}

uint16_t WifiSensingSettings::getSampleIntervalMs() const {
    return _state.sampleIntervalMs;
}

float WifiSensingSettings::getVarianceThreshold() const {
    return _state.varianceThreshold;
}

}  // namespace WIFISENSING
