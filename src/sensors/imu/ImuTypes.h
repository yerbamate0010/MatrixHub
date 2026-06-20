#pragma once

#include <cstdint>

namespace IMU {

struct ImuVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ImuSample {
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;
    uint32_t timestampMs = 0;
    bool valid = false;
};

struct ImuMetrics {
    ImuSample sample;
    uint32_t sampleAgeMs = 0;
    bool sampleTimestampKnown = false;
    bool sampleFresh = false;
    float accelMagnitudeG = 0.0f;
    float accelDeltaG = 0.0f;
    float gyroMagnitudeDps = 0.0f;
    bool baselineValid = false;
    ImuVector3 baseline;
    float tiltDeg = 0.0f;
};

enum class OrientationCalibrationStatus : uint8_t {
    Success = 0,
    NotRunning,
    NoFreshSamples,
    Unstable,
    InvalidVector
};

struct OrientationCalibrationResult {
    OrientationCalibrationStatus status = OrientationCalibrationStatus::NoFreshSamples;
    ImuVector3 baseline;
    float accelMagnitudeMean = 0.0f;
    float accelMagnitudeVariance = 0.0f;
    uint16_t sampleCount = 0;
};

}  // namespace IMU
