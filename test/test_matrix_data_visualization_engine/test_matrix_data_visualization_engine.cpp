#include <unity.h>

#include "../../lib/matrix_service/visualization/MatrixDataVisualizationEngine.cpp"

namespace {

MATRIX::MatrixDataVisualizationConfig baseConfig() {
    MATRIX::MatrixDataVisualizationConfig config;
    config.enabled = true;
    config.minValue = 0.0f;
    config.maxValue = 100.0f;
    config.colorMin = 0x0000FF;
    config.colorMid = 0x00FF00;
    config.colorMax = 0xFF0000;
    config.brightnessMin = 16;
    config.brightnessMax = 160;
    config.smoothing = 0;
    config.staleBehavior = static_cast<uint8_t>(MATRIX::MatrixDataStaleBehavior::Dim);
    return config;
}

}  // namespace

void setUp(void) {}
void tearDown(void) {}

void test_normalize_value_clamps_to_byte_range() {
    TEST_ASSERT_EQUAL_UINT8(0, MATRIX_VIZ::MatrixDataVisualizationEngine::normalizeValue(-10.0f, 0.0f, 100.0f));
    TEST_ASSERT_EQUAL_UINT8(128, MATRIX_VIZ::MatrixDataVisualizationEngine::normalizeValue(50.0f, 0.0f, 100.0f));
    TEST_ASSERT_EQUAL_UINT8(255, MATRIX_VIZ::MatrixDataVisualizationEngine::normalizeValue(150.0f, 0.0f, 100.0f));
}

void test_gauge_renders_expected_number_of_pixels() {
    MATRIX_VIZ::MatrixDataVisualizationEngine engine;
    auto config = baseConfig();
    config.mode = static_cast<uint8_t>(MATRIX::MatrixDataVizMode::Gauge);
    engine.configure(config);

    MATRIX::MatrixDataVisualizationInput input;
    input.valid = true;
    input.value = 50.0f;
    input.timestampMs = 1;
    engine.setInput(input);

    uint32_t frame[64] = {};
    TEST_ASSERT_TRUE(engine.render(100, frame, 64));

    uint8_t lit = 0;
    for (uint32_t pixel : frame) {
        if (pixel != 0) {
            lit++;
        }
    }
    TEST_ASSERT_EQUAL_UINT8(32, lit);
}

void test_stale_blank_behavior_turns_frame_off() {
    MATRIX_VIZ::MatrixDataVisualizationEngine engine;
    auto config = baseConfig();
    config.staleBehavior = static_cast<uint8_t>(MATRIX::MatrixDataStaleBehavior::Blank);
    engine.configure(config);

    MATRIX::MatrixDataVisualizationInput input;
    input.valid = false;
    input.stale = true;
    engine.setInput(input);

    uint32_t frame[64] = {};
    TEST_ASSERT_TRUE(engine.render(100, frame, 64));
    for (uint32_t pixel : frame) {
        TEST_ASSERT_EQUAL_HEX32(0x000000, pixel);
    }
}

void test_heatmap_is_deterministic_for_same_bins() {
    MATRIX_VIZ::MatrixDataVisualizationEngine engine;
    auto config = baseConfig();
    config.mode = static_cast<uint8_t>(MATRIX::MatrixDataVizMode::Heatmap);
    engine.configure(config);

    MATRIX::MatrixDataVisualizationInput input;
    input.valid = true;
    input.value = 25.0f;
    input.timestampMs = 1;
    input.binCount = 64;
    for (uint8_t i = 0; i < 64; ++i) {
        input.bins[i] = static_cast<uint8_t>(i * 4);
    }
    engine.setInput(input);

    uint32_t frameA[64] = {};
    uint32_t frameB[64] = {};
    TEST_ASSERT_TRUE(engine.render(100, frameA, 64));
    TEST_ASSERT_TRUE(engine.render(100, frameB, 64));
    for (uint8_t i = 0; i < 64; ++i) {
        TEST_ASSERT_EQUAL_HEX32(frameA[i], frameB[i]);
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_normalize_value_clamps_to_byte_range);
    RUN_TEST(test_gauge_renders_expected_number_of_pixels);
    RUN_TEST(test_stale_blank_behavior_turns_frame_off);
    RUN_TEST(test_heatmap_is_deterministic_for_same_bins);
    return UNITY_END();
}
