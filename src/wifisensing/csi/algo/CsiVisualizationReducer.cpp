#include "CsiVisualizationReducer.h"

#include <cmath>
#include <cstring>

namespace WIFISENSING {
namespace CSI {
namespace {

constexpr float kGainMin = 0.25f;
constexpr float kGainMax = 4.0f;

float clampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

} // namespace

void CsiVisualizationReducer::reset() {
    _hasSmoothed = false;
    _lastWidth = 0;
    std::memset(_smoothed, 0, sizeof(_smoothed));
    _snapshot = CsiVisualizationSnapshot{};
}

CsiVisualizationSnapshot CsiVisualizationReducer::process(const CsiPacket& packet, uint32_t nowMs) {
    const uint16_t width = static_cast<uint16_t>(packet.len / 2u);
    if (width == 0 || width > (MAX_CSI_DATA_LEN / 2u)) {
        reset();
        _snapshot.timestampMs = nowMs;
        return _snapshot;
    }

    float graph[CSI_VISUALIZATION_BIN_COUNT] = {};
    float minGraph = 0.0f;
    float maxGraph = 0.0f;
    bool hasGraph = false;
    const float gain = sanitizeGain(packet.compensate_gain);
    const float gainSquared = gain * gain;

    for (uint8_t bin = 0; bin < CSI_VISUALIZATION_BIN_COUNT; ++bin) {
        const uint16_t start = static_cast<uint16_t>(
            (static_cast<uint32_t>(bin) * width) / CSI_VISUALIZATION_BIN_COUNT);
        uint16_t end = static_cast<uint16_t>(
            (static_cast<uint32_t>(bin + 1u) * width) / CSI_VISUALIZATION_BIN_COUNT);
        if (end <= start) {
            end = static_cast<uint16_t>(start + 1u);
        }
        if (end > width) {
            end = width;
        }

        float sum = 0.0f;
        uint16_t count = 0;
        for (uint16_t i = start; i < end; ++i) {
            const float re = static_cast<float>(packet.buf[2u * i]);
            const float im = static_cast<float>(packet.buf[(2u * i) + 1u]);
            const float energy = (re * re + im * im) * gainSquared;
            if (!std::isfinite(energy)) {
                continue;
            }
            sum += energy;
            count++;
        }

        const float avgEnergy = count > 0 ? sum / static_cast<float>(count) : 0.0f;
        const float graphValue = std::log1p(avgEnergy);
        graph[bin] = graphValue;
        if (!hasGraph) {
            minGraph = graphValue;
            maxGraph = graphValue;
            hasGraph = true;
        } else {
            if (graphValue < minGraph) {
                minGraph = graphValue;
            }
            if (graphValue > maxGraph) {
                maxGraph = graphValue;
            }
        }
    }

    if (!hasGraph) {
        reset();
        _snapshot.timestampMs = nowMs;
        return _snapshot;
    }

    const bool resetSmoothing = !_hasSmoothed || _lastWidth != width;
    const float range = maxGraph - minGraph;
    uint16_t binSum = 0;

    _snapshot = CsiVisualizationSnapshot{};
    _snapshot.valid = true;
    _snapshot.stale = false;
    _snapshot.timestampMs = nowMs;
    _snapshot.width = width;
    _snapshot.binCount = CSI_VISUALIZATION_BIN_COUNT;

    for (uint8_t bin = 0; bin < CSI_VISUALIZATION_BIN_COUNT; ++bin) {
        float normalized = 0.0f;
        if (range > 0.0001f) {
            normalized = (graph[bin] - minGraph) / range;
        } else if (maxGraph > 0.0001f) {
            normalized = 0.5f;
        }
        const float byteValue = clampFloat(normalized, 0.0f, 1.0f) * 255.0f;
        if (resetSmoothing) {
            _smoothed[bin] = byteValue;
        } else {
            _smoothed[bin] += (byteValue - _smoothed[bin]) * kEwmaAlpha;
        }
        const uint8_t out = clampByte(static_cast<int>(std::lround(_smoothed[bin])));
        _snapshot.bins[bin] = out;
        binSum = static_cast<uint16_t>(binSum + out);
    }

    _snapshot.value = (static_cast<float>(binSum) * 100.0f) /
                      (255.0f * static_cast<float>(CSI_VISUALIZATION_BIN_COUNT));
    _hasSmoothed = true;
    _lastWidth = width;
    return _snapshot;
}

uint8_t CsiVisualizationReducer::clampByte(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return static_cast<uint8_t>(value);
}

float CsiVisualizationReducer::sanitizeGain(float gain) {
    if (!std::isfinite(gain)) {
        gain = 1.0f;
    }
    return clampFloat(gain, kGainMin, kGainMax);
}

} // namespace CSI
} // namespace WIFISENSING
