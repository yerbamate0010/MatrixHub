#include "ImuManager.h"
#include "ImuService.h"
#include "../../system/logging/Logging.h"
#include "../../config/System.h"
#include <Arduino.h>
#include <cstdio>
#include <cstring>

#undef LOG_TAG
#define LOG_TAG "IMU"

namespace IMU {

namespace {
const char* consumerName(Consumer consumer) {
    switch (consumer) {
        case Consumer::AirMouseMovement: return "AirMouseMovement";
        case Consumer::AirMouseClick: return "AirMouseClick";
        case Consumer::AutoRotate: return "AutoRotate";
        default: return "Unknown";
    }
}

void appendLabel(char* out, size_t len, const char* label, bool& first) {
    if (!out || len == 0 || !label) return;
    size_t used = strlen(out);
    if (used >= len - 1) return;
    if (!first) {
        int written = snprintf(out + used, len - used, ",");
        if (written < 0) return;
        used = strlen(out);
    }
    snprintf(out + used, len - used, "%s", label);
    first = false;
}

void formatConsumers(uint32_t mask, char* out, size_t len) {
    if (!out || len == 0) return;
    out[0] = '\0';
    bool first = true;

    if (mask & (1u << static_cast<uint8_t>(Consumer::AirMouseMovement))) {
        appendLabel(out, len, consumerName(Consumer::AirMouseMovement), first);
    }
    if (mask & (1u << static_cast<uint8_t>(Consumer::AirMouseClick))) {
        appendLabel(out, len, consumerName(Consumer::AirMouseClick), first);
    }
    if (mask & (1u << static_cast<uint8_t>(Consumer::AutoRotate))) {
        appendLabel(out, len, consumerName(Consumer::AutoRotate), first);
    }

    if (out[0] == '\0') {
        snprintf(out, len, "none");
    }
}
} // namespace

ImuManager::ImuManager(ImuService* imuService)
    : _imuService(imuService) {
    _transitionMutex = xSemaphoreCreateMutex();
    if (!_transitionMutex) {
        LOGE("IMU manager mutex alloc failed");
    }
}

ImuManager::~ImuManager() {
    if (_transitionMutex) {
        vSemaphoreDelete(_transitionMutex);
        _transitionMutex = nullptr;
    }
}

void ImuManager::setConsumerActive(Consumer consumer, bool active) {
    if (!_imuService) return;

    const uint32_t bitMask = consumerBit(consumer);
    uint32_t prev = 0;
    uint32_t next = 0;
    bool shouldStart = false;
    bool shouldStop = false;
    if (_transitionMutex) {
        if (xSemaphoreTake(_transitionMutex, pdMS_TO_TICKS(CONFIG::IMU::MANAGER_TRANSITION_TIMEOUT_MS)) != pdTRUE) {
            LOGW("IMU transition lock timeout (consumer=%s)", consumerName(consumer));
            return;
        }
    }

    prev = _activeMask.load(std::memory_order_relaxed);
    next = active ? (prev | bitMask) : (prev & ~bitMask);
    if (prev == next) {
        if (_transitionMutex) xSemaphoreGive(_transitionMutex);
        return;
    }

    _activeMask.store(next, std::memory_order_release);
    shouldStart = (prev == 0 && next != 0);
    shouldStop = (prev != 0 && next == 0);
    if (_transitionMutex) xSemaphoreGive(_transitionMutex);

    char activeList[64];
    formatConsumers(next, activeList, sizeof(activeList));
    LOGI("IMU consumer %s -> %s (active=[%s], mask=0x%lx)",
         consumerName(consumer),
         active ? "ON" : "OFF",
         activeList,
         (unsigned long)next);

    if (shouldStart) {
        if (_activeMask.load(std::memory_order_acquire) == 0) {
            LOGW("IMU start skipped (no active consumers)");
            return;
        }
        LOGI("Starting IMU (reason=[%s])", activeList);
        const uint32_t startMs = millis();
        const bool started = _imuService->begin();
        const uint32_t elapsedMs = millis() - startMs;
        if (!started) {
            LOGE("IMU start failed (reason=[%s], %lu ms)", activeList, (unsigned long)elapsedMs);
        } else {
            LOGI("IMU start ok (%lu ms)", (unsigned long)elapsedMs);
        }
    } else if (shouldStop) {
        if (_activeMask.load(std::memory_order_acquire) != 0) {
            LOGW("IMU stop skipped (new consumers active)");
            return;
        }
        char prevList[64];
        formatConsumers(prev, prevList, sizeof(prevList));
        LOGI("Stopping IMU (previous=[%s])", prevList);
        const bool wasRunning = _imuService->isInitialized();
        const uint32_t startMs = millis();
        _imuService->stop();
        const uint32_t elapsedMs = millis() - startMs;
        if (wasRunning) {
            LOGI("IMU resources released (%lu ms)", (unsigned long)elapsedMs);
        } else {
            LOGI("IMU already stopped (%lu ms)", (unsigned long)elapsedMs);
        }
    }
}

} // namespace IMU
