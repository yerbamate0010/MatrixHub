#pragma once

#include <FS.h>
#include <memory>

class ImuService;
namespace IMU {
class ImuManager;
class ImuRuntimeService;
}

/**
 * @brief IMU services initialization
 *
 * Creates ImuService and ImuManager without starting the sensor.
 */
class ImuServicesInitializer {
public:
    struct State {
        std::unique_ptr<ImuService>& imuService;
        std::unique_ptr<IMU::ImuManager>& imuManager;
        std::unique_ptr<IMU::ImuRuntimeService>& imuRuntimeService;
        FS* fs = nullptr;
    };

    static void initialize(State state);
};
