#pragma once

#include <memory>

class ImuService;
namespace IMU { class ImuManager; }

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
    };

    static void initialize(State state);
};
