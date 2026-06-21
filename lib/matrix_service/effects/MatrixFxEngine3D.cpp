#include "MatrixFxEngine3D.h"

#include <algorithm>
#include <cmath>

namespace MATRIX_FX {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinDtSec = 0.016f;
constexpr float kMaxDtSec = 0.120f;

float clampf(float value, float lo, float hi) {
    return std::min(hi, std::max(lo, value));
}

float fract(float value) {
    return value - std::floor(value);
}

uint8_t channel(uint32_t color, uint8_t shift) {
    return static_cast<uint8_t>((color >> shift) & 0xFF);
}

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8) |
           static_cast<uint32_t>(b);
}

}  // namespace

MatrixFxEngine3D::MatrixFxEngine3D() {
    initParticles();
}

void MatrixFxEngine3D::configure(const MatrixFxConfig& config) {
    _config = config;
    _config.mode = normalizeNative3DMode(config.mode);
    _config.provider = static_cast<ReactiveProvider>(
        normalizeReactiveProvider(static_cast<uint8_t>(config.provider)));
    _config.reactivityGain = clampReactivityGain(config.reactivityGain);
    _configured = true;
    _lastRenderMs = 0;
    _lastDeltaSec = 0.031f;
    _phase = 0.0f;
    _tiltX = 0.0f;
    _tiltY = 0.0f;
    initParticles();
}

void MatrixFxEngine3D::setInput(const MatrixFxInput& input) {
    _input = input;
}

void MatrixFxEngine3D::reset() {
    _configured = false;
    _lastRenderMs = 0;
    _lastDeltaSec = 0.031f;
    _phase = 0.0f;
    _tiltX = 0.0f;
    _tiltY = 0.0f;
    initParticles();
}

bool MatrixFxEngine3D::render(uint32_t nowMs, uint32_t* outFrame, uint8_t pixelCount) {
    if (!_configured || !outFrame || pixelCount < kMatrixFxPixelCount) {
        return false;
    }

    const uint32_t deltaMs = _lastRenderMs == 0 ? 31 : nowMs - _lastRenderMs;
    _lastRenderMs = nowMs;
    const float dtSec = clampf(static_cast<float>(deltaMs) / 1000.0f, kMinDtSec, kMaxDtSec);
    _lastDeltaSec = dtSec;
    const float speedScale = clampf(1000.0f / static_cast<float>(std::max<uint32_t>(_config.speedMs, 50)), 0.08f, 20.0f);
    _phase = fract(_phase + dtSec * speedScale * 0.18f);
    updateInputSmoothing(dtSec);

    switch (static_cast<Native3DMode>(_config.mode)) {
        case Native3DMode::GyroCube:
            renderGyroCube(outFrame);
            break;
        case Native3DMode::GravityParticles:
            renderGravityParticles(outFrame);
            break;
        case Native3DMode::DepthTunnel:
            renderDepthTunnel(outFrame);
            break;
        case Native3DMode::LiquidWave:
        default:
            renderLiquidWave(outFrame);
            break;
    }
    return true;
}

void MatrixFxEngine3D::initParticles() {
    for (uint8_t i = 0; i < kParticleCount; ++i) {
        _particles[i].x = 0.5f + static_cast<float>((i * 5) % 7);
        _particles[i].y = 0.5f + static_cast<float>((i * 3) % 7);
        _particles[i].vx = (i % 2 == 0 ? 0.20f : -0.16f) + static_cast<float>(i) * 0.008f;
        _particles[i].vy = (i % 3 == 0 ? -0.18f : 0.14f) - static_cast<float>(i) * 0.006f;
        _particles[i].color = palette(static_cast<float>(i) / static_cast<float>(kParticleCount));
    }
    _particlesInitialized = true;
}

void MatrixFxEngine3D::updateInputSmoothing(float dtSec) {
    const bool useImu = _config.provider == ReactiveProvider::Imu && _input.imuValid;
    const float targetTiltX = useImu ? clampf(_input.ax, -1.0f, 1.0f) : 0.0f;
    const float targetTiltY = useImu ? clampf(_input.ay, -1.0f, 1.0f) : 0.0f;
    const float alpha = clampf(dtSec * 8.0f, 0.05f, 0.65f);
    _tiltX += (targetTiltX - _tiltX) * alpha;
    _tiltY += (targetTiltY - _tiltY) * alpha;
}

float MatrixFxEngine3D::reactiveGain() const {
    return static_cast<float>(_config.reactivityGain) / 100.0f;
}

void MatrixFxEngine3D::clearFrame(uint32_t* outFrame, uint32_t color) const {
    for (uint8_t i = 0; i < kMatrixFxPixelCount; ++i) {
        outFrame[i] = color;
    }
}

void MatrixFxEngine3D::fadeFrame(uint32_t* outFrame, uint8_t amount) const {
    const float scale = clampf(1.0f - static_cast<float>(amount) / 255.0f, 0.0f, 1.0f);
    for (uint8_t i = 0; i < kMatrixFxPixelCount; ++i) {
        outFrame[i] = scaleColor(outFrame[i], scale);
    }
}

uint32_t MatrixFxEngine3D::scaleColor(uint32_t color, float scale) const {
    scale = clampf(scale, 0.0f, 1.0f);
    return rgb(
        static_cast<uint8_t>(static_cast<float>(channel(color, 16)) * scale),
        static_cast<uint8_t>(static_cast<float>(channel(color, 8)) * scale),
        static_cast<uint8_t>(static_cast<float>(channel(color, 0)) * scale));
}

uint32_t MatrixFxEngine3D::blend(uint32_t base, uint32_t add, float alpha) const {
    alpha = clampf(alpha, 0.0f, 1.0f);
    const uint16_t r = std::min<uint16_t>(255, channel(base, 16) + static_cast<uint16_t>(channel(add, 16) * alpha));
    const uint16_t g = std::min<uint16_t>(255, channel(base, 8) + static_cast<uint16_t>(channel(add, 8) * alpha));
    const uint16_t b = std::min<uint16_t>(255, channel(base, 0) + static_cast<uint16_t>(channel(add, 0) * alpha));
    return rgb(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
}

uint32_t MatrixFxEngine3D::mixColor(uint32_t a, uint32_t b, float t) const {
    t = clampf(t, 0.0f, 1.0f);
    const float u = 1.0f - t;
    return rgb(
        static_cast<uint8_t>(channel(a, 16) * u + channel(b, 16) * t),
        static_cast<uint8_t>(channel(a, 8) * u + channel(b, 8) * t),
        static_cast<uint8_t>(channel(a, 0) * u + channel(b, 0) * t));
}

uint32_t MatrixFxEngine3D::palette(float t) const {
    t = fract(t);
    if (t < 0.5f) {
        return mixColor(_config.color1, _config.color2, t * 2.0f);
    }
    return mixColor(_config.color2, _config.color3, (t - 0.5f) * 2.0f);
}

void MatrixFxEngine3D::addPixel(uint32_t* outFrame, int x, int y, uint32_t color, float alpha) const {
    if (x < 0 || x >= kMatrixFxWidth || y < 0 || y >= kMatrixFxHeight) {
        return;
    }
    const uint8_t index = static_cast<uint8_t>(y * kMatrixFxWidth + x);
    outFrame[index] = blend(outFrame[index], color, alpha);
}

void MatrixFxEngine3D::drawLine(uint32_t* outFrame, int x0, int y0, int x1, int y1, uint32_t color, float alpha) const {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        addPixel(outFrame, x0, y0, color, alpha);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void MatrixFxEngine3D::drawRect(uint32_t* outFrame, int left, int top, int right, int bottom, uint32_t color, float alpha) const {
    if (left > right || top > bottom) {
        return;
    }
    drawLine(outFrame, left, top, right, top, color, alpha);
    drawLine(outFrame, right, top, right, bottom, color, alpha);
    drawLine(outFrame, right, bottom, left, bottom, color, alpha);
    drawLine(outFrame, left, bottom, left, top, color, alpha);
}

void MatrixFxEngine3D::renderGyroCube(uint32_t* outFrame) {
    clearFrame(outFrame);

    const bool useImu = _config.provider == ReactiveProvider::Imu && _input.imuValid;
    const float gain = reactiveGain();
    const float gyroSpin = useImu ? clampf(_input.gz * 0.0035f * gain, -0.9f, 0.9f) : 0.0f;
    const float idle = _phase * 2.0f * kPi;
    const float angleX = _tiltY * 0.95f * gain + std::sin(idle) * 0.16f;
    const float angleY = -_tiltX * 0.95f * gain + std::cos(idle * 0.8f) * 0.18f;
    const float angleZ = idle * 0.28f + gyroSpin;
    const float cx = std::cos(angleX);
    const float sx = std::sin(angleX);
    const float cy = std::cos(angleY);
    const float sy = std::sin(angleY);
    const float cz = std::cos(angleZ);
    const float sz = std::sin(angleZ);

    struct Point2 {
        int x;
        int y;
        float z;
    };
    Point2 projected[8]{};
    static constexpr float vertices[8][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1}
    };
    static constexpr uint8_t edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    for (uint8_t i = 0; i < 8; ++i) {
        float x = vertices[i][0];
        float y = vertices[i][1];
        float z = vertices[i][2];

        const float y1 = y * cx - z * sx;
        const float z1 = y * sx + z * cx;
        const float x2 = x * cy + z1 * sy;
        const float z2 = -x * sy + z1 * cy;
        const float x3 = x2 * cz - y1 * sz;
        const float y3 = x2 * sz + y1 * cz;

        const float perspective = 2.35f / (3.3f - z2);
        projected[i].x = static_cast<int>(std::round(3.5f + x3 * perspective * 2.0f));
        projected[i].y = static_cast<int>(std::round(3.5f + y3 * perspective * 2.0f));
        projected[i].z = z2;
    }

    for (const auto& edge : edges) {
        const Point2& a = projected[edge[0]];
        const Point2& b = projected[edge[1]];
        const float depth = clampf((a.z + b.z + 2.0f) * 0.25f, 0.20f, 1.0f);
        drawLine(outFrame, a.x, a.y, b.x, b.y, palette(0.15f + depth * 0.65f), 0.22f + depth * 0.78f);
    }

    for (const auto& point : projected) {
        const float depth = clampf((point.z + 1.0f) * 0.5f, 0.25f, 1.0f);
        addPixel(outFrame, point.x, point.y, _config.color2, 0.35f + depth * 0.55f);
    }
}

void MatrixFxEngine3D::renderGravityParticles(uint32_t* outFrame) {
    if (!_particlesInitialized) {
        initParticles();
    }

    fadeFrame(outFrame, 88);
    const float dt = _lastDeltaSec;
    const float frameScale = dt * 20.0f;
    const float gain = reactiveGain();
    const float gravityX = -_tiltX * gain * 5.2f;
    const float gravityY = -_tiltY * gain * 5.2f;
    const float idleSpin = std::sin(_phase * 2.0f * kPi) * 0.08f;

    for (uint8_t i = 0; i < kParticleCount; ++i) {
        Particle& p = _particles[i];
        p.vx += (gravityX + idleSpin) * dt;
        p.vy += (gravityY - idleSpin) * dt;
        p.vx *= 0.90f;
        p.vy *= 0.90f;
        p.x += p.vx * frameScale;
        p.y += p.vy * frameScale;

        if (p.x < 0.0f) {
            p.x = 0.0f;
            p.vx = std::abs(p.vx) * 0.70f;
        } else if (p.x > 7.0f) {
            p.x = 7.0f;
            p.vx = -std::abs(p.vx) * 0.70f;
        }
        if (p.y < 0.0f) {
            p.y = 0.0f;
            p.vy = std::abs(p.vy) * 0.70f;
        } else if (p.y > 7.0f) {
            p.y = 7.0f;
            p.vy = -std::abs(p.vy) * 0.70f;
        }

        const float energy = clampf(std::sqrt(p.vx * p.vx + p.vy * p.vy) * 2.2f, 0.25f, 1.0f);
        addPixel(outFrame, static_cast<int>(std::round(p.x)), static_cast<int>(std::round(p.y)), p.color, energy);
    }
}

void MatrixFxEngine3D::renderDepthTunnel(uint32_t* outFrame) {
    clearFrame(outFrame);

    const float gain = reactiveGain();
    const float centerX = 3.5f - _tiltX * 1.35f * gain;
    const float centerY = 3.5f - _tiltY * 1.35f * gain;

    for (uint8_t layer = 0; layer < 4; ++layer) {
        const float depth = fract(_phase * 1.55f + static_cast<float>(layer) * 0.25f);
        const float halfSize = 0.35f + depth * 3.25f;
        const int left = static_cast<int>(std::round(centerX - halfSize));
        const int right = static_cast<int>(std::round(centerX + halfSize));
        const int top = static_cast<int>(std::round(centerY - halfSize));
        const int bottom = static_cast<int>(std::round(centerY + halfSize));
        const float alpha = clampf(0.18f + depth * 0.78f, 0.18f, 0.96f);
        drawRect(outFrame, left, top, right, bottom, palette(depth * 0.7f + 0.1f), alpha);
    }

    addPixel(outFrame, static_cast<int>(std::round(centerX)), static_cast<int>(std::round(centerY)), _config.color3, 0.65f);
}

void MatrixFxEngine3D::renderLiquidWave(uint32_t* outFrame) {
    const float gain = reactiveGain();
    const float waveTiltX = -_tiltX * 0.65f * gain;
    const float waveTiltY = -_tiltY * 0.65f * gain;

    for (uint8_t y = 0; y < kMatrixFxHeight; ++y) {
        for (uint8_t x = 0; x < kMatrixFxWidth; ++x) {
            const float nx = (static_cast<float>(x) - 3.5f) / 3.5f;
            const float ny = static_cast<float>(y) / 7.0f;
            const float perspective = 0.55f + ny * 1.65f;
            const float surfaceX = nx * perspective + waveTiltX;
            const float surfaceY = ny + waveTiltY * 0.35f;
            const float wave =
                std::sin(surfaceX * 3.1f + _phase * 2.0f * kPi + surfaceY * 1.6f) * (0.18f + ny * 0.26f);
            const float band = 1.0f - std::abs(fract((surfaceY + wave) * 4.0f - _phase * 1.15f) - 0.5f) * 2.0f;
            const float depthLight = 0.16f + ny * 0.38f;
            const float bright = clampf(depthLight + band * (0.28f + ny * 0.34f), 0.05f, 1.0f);
            outFrame[y * kMatrixFxWidth + x] = scaleColor(palette(0.48f + surfaceY * 0.28f + wave * 0.25f), bright);
        }
    }
}

}  // namespace MATRIX_FX
