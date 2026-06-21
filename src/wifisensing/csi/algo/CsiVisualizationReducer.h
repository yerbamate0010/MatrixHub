#pragma once

#include <cstdint>

#include "../data/CsiTypes.h"
#include "CsiVisualizationTypes.h"

namespace WIFISENSING {
namespace CSI {

class CsiVisualizationReducer {
public:
    void reset();
    CsiVisualizationSnapshot process(const CsiPacket& packet, uint32_t nowMs);
    CsiVisualizationSnapshot snapshot() const { return _snapshot; }

private:
    static constexpr float kEwmaAlpha = 0.30f;

    static uint8_t clampByte(int value);
    static float sanitizeGain(float gain);

    float _smoothed[CSI_VISUALIZATION_BIN_COUNT] = {};
    bool _hasSmoothed = false;
    uint16_t _lastWidth = 0;
    CsiVisualizationSnapshot _snapshot;
};

} // namespace CSI
} // namespace WIFISENSING
