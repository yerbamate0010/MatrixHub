#include "ImuServicesInitializer.h"

#include "../../../sensors/imu/ImuService.h"
#include "../../../sensors/imu/ImuManager.h"
#include "../../logging/Logging.h"

#undef LOG_TAG
#define LOG_TAG "ImuInit"

void ImuServicesInitializer::initialize(State state) {
    state.imuService = std::make_unique<ImuService>();
    state.imuManager = std::make_unique<IMU::ImuManager>(state.imuService.get());

    LOGI("IMU services initialized (idle)");
}
