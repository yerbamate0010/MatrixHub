#include <unity.h>

#include "../../lib/matrix_service/effects/MatrixFxEngine3D.cpp"

namespace {

MATRIX_FX::MatrixFxConfig makeConfig(uint8_t mode) {
    MATRIX_FX::MatrixFxConfig config;
    config.mode = mode;
    config.speedMs = 900;
    config.color1 = 0x1030FF;
    config.color2 = 0xFF3050;
    config.color3 = 0x20FF80;
    config.provider = MATRIX_FX::ReactiveProvider::Imu;
    config.reactivityGain = 120;
    return config;
}

uint32_t countLitPixels(const uint32_t* frame) {
    uint32_t count = 0;
    for (uint8_t i = 0; i < MATRIX_FX::kMatrixFxPixelCount; ++i) {
        if ((frame[i] & 0x00FFFFFFu) != 0) {
            count++;
        }
    }
    return count;
}

uint32_t countChangedPixels(const uint32_t* a, const uint32_t* b) {
    uint32_t count = 0;
    for (uint8_t i = 0; i < MATRIX_FX::kMatrixFxPixelCount; ++i) {
        if ((a[i] & 0x00FFFFFFu) != (b[i] & 0x00FFFFFFu)) {
            count++;
        }
    }
    return count;
}

uint8_t brightness(uint32_t color) {
    const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
    const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    const uint8_t b = static_cast<uint8_t>(color & 0xFF);
    return static_cast<uint8_t>((static_cast<uint16_t>(r) + g + b) / 3);
}

float weightedY(const uint32_t* frame) {
    float sum = 0.0f;
    float weight = 0.0f;
    for (uint8_t y = 0; y < MATRIX_FX::kMatrixFxHeight; ++y) {
        for (uint8_t x = 0; x < MATRIX_FX::kMatrixFxWidth; ++x) {
            const float b = static_cast<float>(brightness(frame[y * MATRIX_FX::kMatrixFxWidth + x]));
            sum += static_cast<float>(y) * b;
            weight += b;
        }
    }
    return weight > 0.0f ? sum / weight : 0.0f;
}

}  // namespace

void setUp(void) {}
void tearDown(void) {}

void test_engine_rejects_render_before_configure() {
    MATRIX_FX::MatrixFxEngine3D engine;
    uint32_t frame[MATRIX_FX::kMatrixFxPixelCount]{};

    TEST_ASSERT_FALSE(engine.render(100, frame, MATRIX_FX::kMatrixFxPixelCount));
}

void test_engine_renders_all_native_modes_to_nonblank_frames() {
    for (uint8_t mode = 0; mode <= MATRIX_FX::kNative3DModeMax; ++mode) {
        MATRIX_FX::MatrixFxEngine3D engine;
        uint32_t frame[MATRIX_FX::kMatrixFxPixelCount]{};

        engine.configure(makeConfig(mode));

        TEST_ASSERT_TRUE(engine.render(1000 + mode * 40, frame, MATRIX_FX::kMatrixFxPixelCount));
        TEST_ASSERT_GREATER_THAN_UINT32(0, countLitPixels(frame));
    }
}

void test_engine_clamps_config_and_keeps_small_runtime_state() {
    MATRIX_FX::MatrixFxEngine3D engine;
    uint32_t frame[MATRIX_FX::kMatrixFxPixelCount]{};
    auto config = makeConfig(255);
    config.provider = static_cast<MATRIX_FX::ReactiveProvider>(255);
    config.reactivityGain = 255;

    engine.configure(config);

    TEST_ASSERT_TRUE(engine.render(1000, frame, MATRIX_FX::kMatrixFxPixelCount));
    TEST_ASSERT_GREATER_THAN_UINT32(0, countLitPixels(frame));
    TEST_ASSERT_LESS_THAN_UINT32(512, static_cast<uint32_t>(sizeof(MATRIX_FX::MatrixFxEngine3D)));
}

void test_imu_input_changes_projected_frame() {
    const auto config = makeConfig(static_cast<uint8_t>(MATRIX_FX::Native3DMode::DepthTunnel));
    uint32_t idleFrame[MATRIX_FX::kMatrixFxPixelCount]{};
    uint32_t reactiveFrame[MATRIX_FX::kMatrixFxPixelCount]{};

    MATRIX_FX::MatrixFxEngine3D idle;
    idle.configure(config);
    TEST_ASSERT_TRUE(idle.render(1000, idleFrame, MATRIX_FX::kMatrixFxPixelCount));

    MATRIX_FX::MatrixFxEngine3D reactive;
    reactive.configure(config);
    MATRIX_FX::MatrixFxInput input;
    input.imuValid = true;
    input.ax = 0.85f;
    input.ay = -0.65f;
    input.az = 0.40f;
    input.gx = 120.0f;
    input.gy = -80.0f;
    input.gz = 35.0f;
    reactive.setInput(input);
    TEST_ASSERT_TRUE(reactive.render(1000, reactiveFrame, MATRIX_FX::kMatrixFxPixelCount));

    TEST_ASSERT_GREATER_THAN_UINT32(0, countChangedPixels(idleFrame, reactiveFrame));
}

void test_gravity_particles_fall_opposite_accelerometer_support_vector() {
    const auto config = makeConfig(static_cast<uint8_t>(MATRIX_FX::Native3DMode::GravityParticles));
    uint32_t supportUpFrame[MATRIX_FX::kMatrixFxPixelCount]{};
    uint32_t supportDownFrame[MATRIX_FX::kMatrixFxPixelCount]{};

    MATRIX_FX::MatrixFxInput supportUp;
    supportUp.imuValid = true;
    supportUp.ay = 0.85f;

    MATRIX_FX::MatrixFxInput supportDown;
    supportDown.imuValid = true;
    supportDown.ay = -0.85f;

    MATRIX_FX::MatrixFxEngine3D supportUpEngine;
    supportUpEngine.configure(config);
    supportUpEngine.setInput(supportUp);
    for (uint8_t i = 0; i < 24; ++i) {
        TEST_ASSERT_TRUE(supportUpEngine.render(1000 + i * 33, supportUpFrame, MATRIX_FX::kMatrixFxPixelCount));
    }

    MATRIX_FX::MatrixFxEngine3D supportDownEngine;
    supportDownEngine.configure(config);
    supportDownEngine.setInput(supportDown);
    for (uint8_t i = 0; i < 24; ++i) {
        TEST_ASSERT_TRUE(supportDownEngine.render(1000 + i * 33, supportDownFrame, MATRIX_FX::kMatrixFxPixelCount));
    }

    TEST_ASSERT_LESS_THAN_FLOAT(weightedY(supportDownFrame) - 0.45f, weightedY(supportUpFrame));
}

void test_depth_tunnel_keeps_readable_wireframe_shape() {
    const auto config = makeConfig(static_cast<uint8_t>(MATRIX_FX::Native3DMode::DepthTunnel));
    uint32_t frame[MATRIX_FX::kMatrixFxPixelCount]{};
    MATRIX_FX::MatrixFxEngine3D engine;
    engine.configure(config);

    TEST_ASSERT_TRUE(engine.render(1000, frame, MATRIX_FX::kMatrixFxPixelCount));

    const uint32_t litPixels = countLitPixels(frame);
    TEST_ASSERT_GREATER_THAN_UINT32(8, litPixels);
    TEST_ASSERT_LESS_THAN_UINT32(MATRIX_FX::kMatrixFxPixelCount, litPixels);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_engine_rejects_render_before_configure);
    RUN_TEST(test_engine_renders_all_native_modes_to_nonblank_frames);
    RUN_TEST(test_engine_clamps_config_and_keeps_small_runtime_state);
    RUN_TEST(test_imu_input_changes_projected_frame);
    RUN_TEST(test_gravity_particles_fall_opposite_accelerometer_support_vector);
    RUN_TEST(test_depth_tunnel_keeps_readable_wireframe_shape);
    return UNITY_END();
}
