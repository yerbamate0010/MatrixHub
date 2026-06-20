#pragma once

#include "ImuManager.h"
#include "ImuTypes.h"
#include "../../system/rtc/RtcStatefulService.h"
#include "../../system/rtc/types/RtcImuTypes.h"

#include <FS.h>

class ImuService;

namespace IMU {

class ImuRuntimeService : public RtcStatefulService<RTC::ImuData> {
public:
    ImuRuntimeService(FS* fs, ImuService* imuService, ImuManager* imuManager);

    void begin();
    void tick();

    ImuMetrics getMetrics() const;
    ManagerStatus getManagerStatus() const;
    OrientationCalibrationResult calibrateOrientation();

    static void readState(RTC::ImuData& settings, JsonObject& root);
    static StateUpdateResult updateState(
        JsonObject& jsonObject,
        RTC::ImuData& settings,
        std::string_view originId);

private:
    StateHandlerResult persistConfig();
    void applyConsumerState();
    bool updateMetricsFromCache(uint32_t nowMs);
    void publishBaselineFromSettings(const RTC::ImuData& settings);

    FS* _fs = nullptr;
    ImuService* _imuService = nullptr;
    ImuManager* _imuManager = nullptr;
    mutable portMUX_TYPE _metricsMux = portMUX_INITIALIZER_UNLOCKED;
    ImuMetrics _metrics;
    uint32_t _lastMetricUpdateMs = 0;
};

}  // namespace IMU
