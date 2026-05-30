#pragma once

#include <cstdint>
#include "types/MatrixTypes.h"

enum class CommandType {
    NONE = 0,
    CLEAR,
    SHOW_ICON,
    SHOW_TEXT,
    SHOW_SOLID,
    SHOW_EFFECT,
    SET_BRIGHTNESS,
    SET_ROTATION
};

struct MatrixCommand {
    CommandType type = CommandType::NONE;
    IconType icon = IconType::NONE;
    char text[kMatrixTextCapacity] = {0};
    uint32_t color = 0;
    uint32_t durationMs = 0;
    uint8_t value8 = 0;
    uint16_t value16 = 0;
    uint32_t value32 = 0;
    uint32_t value32_2 = 0;
    uint32_t value32_3 = 0;
    bool stopBackground = true;
};
