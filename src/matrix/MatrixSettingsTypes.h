#pragma once

#include "../system/rtc/RtcConfig.h"
#include "MatrixCustomIconStore.h"

namespace MATRIX {

struct MatrixCustomIconsState {
    bool has[kMatrixCustomIconCount] = {false};
    uint32_t icons[kMatrixCustomIconCount][kMatrixCustomIconPixels] = {};
};

struct MatrixSettingsState {
    RTC::MatrixData config{};
    MatrixCustomIconsState customIcons{};
};

}  // namespace MATRIX
