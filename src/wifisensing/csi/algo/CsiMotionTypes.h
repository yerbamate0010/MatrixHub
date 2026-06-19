#pragma once

#include <stdint.h>

#include "../data/CsiTypes.h"

namespace WIFISENSING {
namespace CSI {

constexpr uint8_t MAX_CSI_ALARM_BANDS = 4;
constexpr uint16_t MAX_CSI_SUBCARRIERS = MAX_CSI_DATA_LEN / 2;

static_assert(MAX_CSI_ALARM_BANDS == 4, "CSI alarm UI/config assumes max 4 bands");
static_assert(MAX_CSI_SUBCARRIERS <= 256, "Review CSI motion storage before increasing CSI width");

struct CsiBandRange {
    uint16_t start = 0;
    uint16_t end = 0; // inclusive
};

enum class CsiMotionState : uint8_t {
    Disabled = 0,
    NeedsConfiguration = 1,
    Calibrating = 2,
    Monitoring = 3,
    MotionCandidate = 4,
    MotionConfirmed = 5,
    NoisyEnvironment = 6,
    Unavailable = 7,
};

struct CsiMotionConfig {
    bool enabled = false;
    uint8_t bandCount = 0;
    CsiBandRange bands[MAX_CSI_ALARM_BANDS]{};
    uint16_t baselineFrames = 150;
    uint8_t topK = 8;
    float enterThreshold = 6.0f;
    float clearThreshold = 3.0f;
    uint16_t holdMs = 1200;
    uint16_t clearHoldMs = 2500;
    float minNoise = 4.0f;
    float minEnergy = 4.0f;
    float noisyScoreThreshold = 80.0f;
    bool autoRecalibration = true;
    uint8_t sensitivity = 1;
};

struct CsiMotionSnapshot {
    CsiMotionState state = CsiMotionState::Disabled;
    bool motion = false;
    bool baselineReady = false;
    bool noisy = false;
    bool needsCalibration = false;
    float score = 0.0f;
    float confidence = 0.0f;
    uint32_t framesSeen = 0;
    uint16_t width = 0;
    uint16_t selectedCarrierCount = 0;
    uint16_t validCarrierCount = 0;
    uint8_t bandCount = 0;
};

const char* toString(CsiMotionState state);

} // namespace CSI
} // namespace WIFISENSING
