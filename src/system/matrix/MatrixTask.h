#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

class ImuService;
namespace IMU { class ImuManager; }
class MatrixService;

#include <atomic>

namespace MATRIX_MANAGER { class MatrixManagerService; }

namespace MATRIX {

class MatrixMenuService;

class MatrixTask {
public:
    static void start(MatrixMenuService* menu, ImuService* imuService, IMU::ImuManager* imuManager, MatrixService* matrixService, MATRIX_MANAGER::MatrixManagerService* matrixManager);
    static void stop();

private:
    static void taskLoop(void* param);
    static void evaluateAutoRotation(ImuService* imuService, IMU::ImuManager* imuManager, MatrixService* matrixService);
    static void evaluateEffectInput(ImuService* imuService, IMU::ImuManager* imuManager, MatrixService* matrixService);
    static void resetAutoRotationState();
    static bool reapStoppedTask(TickType_t waitTicks);
    static void destroyTaskResources();
    
    static TaskHandle_t _taskHandle;
    static StackType_t* _taskStack;
    static StaticTask_t* _taskBuffer;
    static SemaphoreHandle_t _stopAck;
    static std::atomic<bool> _isRunning;
    static uint32_t _lastImuCheckMs;
    static bool _lastAutoRotateEnabled;
    static uint8_t _lastAppliedAutoRotation;
    static bool _lastMatrixEffectsImuEnabled;
    
    struct TaskParams {
        MatrixMenuService* menu;
        ImuService* imuService;
        IMU::ImuManager* imuManager;
        MatrixService* matrixService;
        MATRIX_MANAGER::MatrixManagerService* matrixManager;
    };
};

} // namespace MATRIX
