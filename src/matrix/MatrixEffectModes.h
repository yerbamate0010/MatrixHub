#pragma once

#include <cstdint>

#include "../config/UiConfig.h"

namespace MATRIX {

constexpr uint8_t kMatrixEffectModeMax = 69;

// Matrix effects now use one compact range shared by the UI, config layer and
// renderer, so validation can stay a simple upper-bound check.
constexpr bool isSupportedMatrixEffectMode(uint8_t mode) {
    return mode <= kMatrixEffectModeMax;
}

// Normalize any out-of-range value back to the safe default so config loads,
// API writes and renderer calls all converge on the same behaviour.
constexpr uint8_t normalizeMatrixEffectMode(uint8_t mode) {
    return isSupportedMatrixEffectMode(mode) ? mode : UI::MATRIX::DEFAULT_EFFECT_MODE;
}

}  // namespace MATRIX
