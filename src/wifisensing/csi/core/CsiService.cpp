#include <Arduino.h>
#include "CsiService.h"
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <new>
#include "../../../system/logging/Logging.h" 
#include "../../../system/utils/ScopeLock.h"
#include "config/System.h"

#undef LOG_TAG
#define LOG_TAG "CsiService"

namespace WIFISENSING {
namespace CSI {

CsiService::CsiService() {
    _stateMutex = xSemaphoreCreateMutex();
    _callbackMutex = xSemaphoreCreateMutex();
}

CsiService::~CsiService() {
    while (true) {
        const bool runtimeResourcesPresent =
            (_processingTaskHandle != nullptr) ||
            (_queue != nullptr) ||
            (_cleanupSem != nullptr) ||
            (_rxCallbackEnabled.load(std::memory_order_acquire)) ||
            (_rxCallbacksInFlight.load(std::memory_order_acquire) != 0);

        if (!_enabled.load(std::memory_order_acquire) && !runtimeResourcesPresent) {
            break;
        }

        // The destructor may run after a partial startup or partial shutdown.
        // Keep funneling everything through the normal disable path until the
        // callback, task, queue and cleanup semaphore are all gone.
        _activeConsumers.store(0, std::memory_order_relaxed);

        // Force disable path to release callback, task, queue and semaphores.
        if (!_enabled.load(std::memory_order_relaxed) && runtimeResourcesPresent) {
            _enabled.store(true, std::memory_order_relaxed);
        }

        if (applyEnabledState(false)) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (_callbackMutex) {
        vSemaphoreDelete(_callbackMutex);
        _callbackMutex = nullptr;
    }
    if (_stateMutex) {
        vSemaphoreDelete(_stateMutex);
        _stateMutex = nullptr;
    }
}

void CsiService::begin() {
    LOGI("Service initialized (Disabled by default)");
    // Queue is now allocated lazily when the first consumer becomes active.
}

void CsiService::setCsiCallback(CsiCallback cb) {
    if (!_callbackMutex) {
        _csiCallback = std::move(cb);
        return;
    }

    SYSTEM::ScopeLock lock(_callbackMutex, pdMS_TO_TICKS(200));
    if (!lock.isLocked()) {
        LOGW("Failed to update CSI callback");
        return;
    }

    _csiCallback = std::move(cb);
}

uint32_t CsiService::consumerBit(CsiConsumer consumer) {
    return 1u << static_cast<uint8_t>(consumer);
}

bool CsiService::hasActiveConsumers() const {
    return _activeConsumers.load(std::memory_order_relaxed) != 0;
}

bool CsiService::isConsumerActive(CsiConsumer consumer) const {
    return (_activeConsumers.load(std::memory_order_relaxed) & consumerBit(consumer)) != 0;
}

bool CsiService::setConsumerActive(CsiConsumer consumer, bool active) {
    SYSTEM::ScopeLock lock(_stateMutex, pdMS_TO_TICKS(200));
    if (!lock.isLocked()) {
        LOGW("Failed to update CSI consumer state");
        return _enabled.load(std::memory_order_relaxed);
    }

    const uint32_t bit = consumerBit(consumer);
    const uint32_t previousMask = _activeConsumers.load(std::memory_order_relaxed);
    uint32_t nextMask = previousMask;

    if (active) {
        nextMask |= bit;
    } else {
        nextMask &= ~bit;
    }

    if (nextMask == previousMask) {
        if ((nextMask != 0) != _enabled.load(std::memory_order_relaxed)) {
            applyEnabledState(nextMask != 0);
        }
        return _enabled.load(std::memory_order_relaxed);
    }

    // Service lifetime follows the aggregate consumer bitmask, not whichever
    // caller toggled last. That way the frontend, boot path and future alarm
    // integrations cannot accidentally disable CSI for one another.
    const bool desiredEnabled = (nextMask != 0);
    const bool currentEnabled = _enabled.load(std::memory_order_relaxed);
    if (desiredEnabled != currentEnabled) {
        if (!applyEnabledState(desiredEnabled)) {
            return _enabled.load(std::memory_order_relaxed);
        }
    }

    _activeConsumers.store(nextMask, std::memory_order_relaxed);
    return _enabled.load(std::memory_order_relaxed);
}

bool CsiService::applyEnabledState(bool enabled) {
    const bool runtimeResourcesPresent =
        (_processingTaskHandle != nullptr) ||
        (_queue != nullptr) ||
        (_cleanupSem != nullptr) ||
        (_rxCallbackEnabled.load(std::memory_order_acquire)) ||
        (_rxCallbacksInFlight.load(std::memory_order_acquire) != 0);

    if (_enabled.load(std::memory_order_relaxed) == enabled) {
        if (!enabled && runtimeResourcesPresent) {
            // Keep going so a partially stopped runtime can be reaped.
        } else {
            return true;
        }
    }

    if (enabled) {
        LOGI("Enabling CSI Sensing (Allocating Resources)...");
        bool csiConfigured = false;
        
        // The queue must exist before the driver callback is registered, otherwise
        // the first CSI frame could arrive while the handoff path is still null.
        if (!_queue) {
            _queue = new CsiDataQueue(16); // 16 packets buffer (~1.6s at 10Hz throttled)
            if (!_queue->begin()) {
                LOGE("Failed to allocate CSI Queue!");
                delete _queue;
                _queue = nullptr;
                return false;
            }
        }

        // 1b. Create Cleanup Semaphore
        if (!_cleanupSem) {
            _cleanupSem = xSemaphoreCreateBinary();
        }
        if (!_cleanupSem) {
            LOGE("Failed to allocate CSI cleanup semaphore");
            rollbackFailedEnable(false);
            return false;
        }

        // Reset Components
        _gainCtrl.reset();
        
        // Adaptive Rate Reset
        _rxFrameCount.store(0, std::memory_order_relaxed);
        _lastRateCheckTime = millis();
        _currentPingInterval = SENSOR::WIFI_SENSING::CSI_PING_INTERVAL_MS;
        _lastRxAcceptTimeUs.store(0, std::memory_order_relaxed);
        _rxCallbacksInFlight.store(0, std::memory_order_relaxed);
        // Open the software gate before registering with the Wi-Fi driver. During
        // shutdown we flip this gate first, then wait for _rxCallbacksInFlight == 0.
        _rxCallbackEnabled.store(true, std::memory_order_release);

        if (!initCsiConfig()) {
            rollbackFailedEnable(false);
            return false;
        }
        csiConfigured = true;
        // Ping traffic is the deliberate producer for repeatable CSI samples. Without
        // it, an idle network can leave the callback with little or no input to process.
        _ping.start();
        if (!_ping.isActive()) {
            LOGE("CSI ping producer failed to start, rolling back enable path");
            rollbackFailedEnable(csiConfigured);
            return false;
        }
        if (!startProcessingTask()) {
            LOGE("CSI consumer task failed to start, rolling back enable path");
            rollbackFailedEnable(csiConfigured);
            return false;
        }

        // Publish "enabled" only after queue, callback, ping and processing task are alive.
        _enabled.store(true, std::memory_order_relaxed);
        return true;
    } else {
        LOGI("Disabling CSI Sensing (Freeing Resources)...");
        _enabled.store(false, std::memory_order_relaxed);

        // Teardown order is deliberate:
        // 1. stop new callback entries and wait for in-flight copies to finish
        // 2. stop the producer side (driver + ping)
        // 3. stop the consumer task
        // 4. destroy the queue and cleanup semaphore
        //
        // Reordering this can turn a clean shutdown into a race with the ISR path.
        _rxCallbackEnabled.store(false, std::memory_order_release);
        const esp_err_t detachErr = esp_wifi_set_csi_rx_cb(nullptr, nullptr);
        if (detachErr != ESP_OK) {
            LOGW("Failed to detach CSI RX callback: %s", esp_err_to_name(detachErr));
        }
        if (!waitForRxCallbacksToDrain(TIMEOUT::TASK_SHUTDOWN_MS)) {
            LOGW("Timed out waiting for in-flight CSI callbacks to drain");
        }

        // 2. Stop producers after the callback path is detached.
        const esp_err_t disableErr = esp_wifi_set_csi(false);
        if (disableErr != ESP_OK) {
            LOGW("Failed to disable CSI driver: %s", esp_err_to_name(disableErr));
        }
        _ping.stop();

        // 3. Stop consumer before deleting the queue it reads from.
        if (!stopProcessingTask()) {
            LOGW("CSI processing task did not suspend cleanly - keeping queue/resources allocated");
            return false;
        }
        
        // 4. Free queue only after the RX callback path is guaranteed inactive.
        if (_queue) {
            delete _queue;
            _queue = nullptr;
        }

        if (_cleanupSem) {
            vSemaphoreDelete(_cleanupSem);
            _cleanupSem = nullptr;
        }

        return true;
    }

    return true;
}

bool CsiService::initCsiConfig() {
    // Keep the common LTF variants enabled so CSI remains available across the
    // PHY modes we see from normal AP traffic. Channel filtering stays on to
    // reduce obviously noisy data before the app-level pipeline sees it.
    wifi_csi_config_t csi_config = {
        .lltf_en           = true,
        .htltf_en          = true,
        .stbc_htltf2_en    = true,
        .ltf_merge_en      = true,
        .channel_filter_en = true, 
        .manu_scale        = false,
        .shift             = false,
    };
    const esp_err_t configErr = esp_wifi_set_csi_config(&csi_config);
    if (configErr != ESP_OK) {
        LOGE("Failed to configure CSI: %s", esp_err_to_name(configErr));
        return false;
    }

    const esp_err_t callbackErr = esp_wifi_set_csi_rx_cb(wifi_csi_rx_cb, this);
    if (callbackErr != ESP_OK) {
        LOGE("Failed to register CSI RX callback: %s", esp_err_to_name(callbackErr));
        return false;
    }

    const esp_err_t enableErr = esp_wifi_set_csi(true);
    if (enableErr != ESP_OK) {
        LOGE("Failed to enable CSI driver: %s", esp_err_to_name(enableErr));
        esp_wifi_set_csi_rx_cb(nullptr, nullptr);
        return false;
    }

    LOGI("CSI Configured and Active");
    return true;
}

void CsiService::rollbackFailedEnable(bool csiConfigured) {
    // Undo partial startup in reverse dependency order so no producer or task can
    // outlive the queue / callback path it expects to use.
    _rxCallbackEnabled.store(false, std::memory_order_release);

    if (csiConfigured) {
        const esp_err_t detachErr = esp_wifi_set_csi_rx_cb(nullptr, nullptr);
        if (detachErr != ESP_OK) {
            LOGW("Rollback: failed to detach CSI RX callback: %s", esp_err_to_name(detachErr));
        }
        if (!waitForRxCallbacksToDrain(TIMEOUT::TASK_SHUTDOWN_MS)) {
            LOGW("Rollback: timed out waiting for CSI callbacks to drain");
        }

        const esp_err_t disableErr = esp_wifi_set_csi(false);
        if (disableErr != ESP_OK) {
            LOGW("Rollback: failed to disable CSI driver: %s", esp_err_to_name(disableErr));
        }
    }

    _ping.stop();
    if (!stopProcessingTask()) {
        LOGW("Rollback: CSI processing task still stopping, deferring queue cleanup");
        return;
    }

    if (_queue) {
        delete _queue;
        _queue = nullptr;
    }

    if (_cleanupSem) {
        vSemaphoreDelete(_cleanupSem);
        _cleanupSem = nullptr;
    }

    _enabled.store(false, std::memory_order_relaxed);
}

} // namespace CSI
} // namespace WIFISENSING
