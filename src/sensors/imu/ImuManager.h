#pragma once

#include <atomic>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class ImuService;

namespace IMU {

enum class Consumer : uint8_t {
    AirMouseMovement = 0,
    AirMouseClick = 1,
    AutoRotate = 2
};

class ImuManager {
public:
    explicit ImuManager(ImuService* imuService);
    ~ImuManager();

    void setConsumerActive(Consumer consumer, bool active);
    bool isActive() const {
        return _activeMask.load(std::memory_order_acquire) != 0;
    }

private:
    ImuService* _imuService = nullptr;
    std::atomic<uint32_t> _activeMask{0};
    SemaphoreHandle_t _transitionMutex = nullptr;

    static constexpr uint32_t consumerBit(Consumer consumer) {
        return 1u << static_cast<uint8_t>(consumer);
    }
};

} // namespace IMU
