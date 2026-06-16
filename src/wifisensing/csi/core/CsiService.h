#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <vector>
#include <atomic>
#include <esp_attr.h>

#include <esp_wifi_types.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/icmp.h> // Ensure standard ICMP header availability
#include "config/System.h"
#include "../data/CsiTypes.h"
#include "../data/CsiDataQueue.h"
#include "CsiPingSession.h"
#include "../algo/CsiGainController.h"

#include <functional>

namespace WIFISENSING {
namespace CSI {

// Callbacks defined in CsiTypes.h

enum class CsiConsumer : uint8_t {
    Frontend = 0,
    AlarmSystem = 1,
    Boot = 2,
};

class CsiService {
public:
    CsiService();
    ~CsiService();

    void begin();
    bool isEnabled() const { return _enabled.load(std::memory_order_relaxed); }
    bool setConsumerActive(CsiConsumer consumer, bool active);
    bool isConsumerActive(CsiConsumer consumer) const;
    bool hasActiveConsumers() const;

    // Register a callback to receive processed CSI packets (for API streaming)
    void setCsiCallback(CsiCallback cb);

    // Components
    CsiPingSession _ping;
    CsiGainController _gainCtrl;

    // State
    std::atomic<bool> _enabled{false};
    std::atomic<bool> _shouldExit{false}; // Graceful shutdown flag
    uint32_t _lastPingTime = 0;

    CsiCallback _csiCallback = nullptr;

    // Processing Task
    bool startProcessingTask();
    bool stopProcessingTask();
    static void processingTask(void* param);

    // Rate Control
    std::atomic<uint32_t> _rxFrameCount{0};
    uint32_t _lastRateCheckTime = 0;
    uint32_t _currentPingInterval = 0;
    std::atomic<uint32_t> _lastRxAcceptTimeUs{0}; // Shared with ISR to enforce pre-queue throttling.

    // RX throttle (60ms = 60,000us) (safe for 10Hz ping with +-20ms jitter)
    static constexpr uint32_t CSI_RX_THROTTLE_INTERVAL_US = 60000;

    // Helpers
    // Ping session now managed via _ping class

    // CSI Callback (Static for C-API compatibility)
    static void IRAM_ATTR wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info);
    
    // Internal initialization helper
    bool initCsiConfig();
    bool applyEnabledState(bool enabled);
    static uint32_t consumerBit(CsiConsumer consumer);
    bool waitForRxCallbacksToDrain(uint32_t timeoutMs);
    void rollbackFailedEnable(bool csiConfigured);
    CsiCallback getCsiCallbackSnapshot();
    bool reapStoppedProcessingTask(TickType_t waitTicks);
    void destroyProcessingTaskResources();

    CsiDataQueue* _queue = nullptr;
    TaskHandle_t _processingTaskHandle = nullptr;
    SemaphoreHandle_t _cleanupSem = nullptr; // Sync for safe shutdown
    SemaphoreHandle_t _stateMutex = nullptr;
    SemaphoreHandle_t _callbackMutex = nullptr;

    std::atomic<uint32_t> _activeConsumers{0};

    // Static Task Storage
    StackType_t* _taskStack = nullptr;
    StaticTask_t* _taskBuffer = nullptr;
    std::atomic<bool> _rxCallbackEnabled{false};
    // Shutdown waits for this counter after detaching the callback so the queue can
    // be destroyed without racing a callback that already started copying data.
    std::atomic<uint32_t> _rxCallbacksInFlight{0};
    
    CsiPacket* _batchBuffer = nullptr;
    static constexpr size_t BATCH_CAPACITY = MAX_CSI_BATCH_PACKETS;

};

} // namespace CSI
} // namespace WIFISENSING
