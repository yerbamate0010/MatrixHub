#include <unity.h>

#include "../../lib/matrix_service/core/MatrixState.cpp"

void setUp(void) {}
void tearDown(void) {}

void test_request_text_preserves_payload_up_to_command_buffer_size() {
    MatrixState state;
    MatrixCommand command;

    char longText[kMatrixTextCapacity];
    memset(longText, 'A', sizeof(longText) - 1);
    longText[sizeof(longText) - 1] = '\0';

    state.requestText(longText, 0x123456, 2500);

    TEST_ASSERT_TRUE(state.poll(command));
    TEST_ASSERT_EQUAL(static_cast<int>(CommandType::SHOW_TEXT), static_cast<int>(command.type));
    TEST_ASSERT_EQUAL_STRING(longText, command.text);
    TEST_ASSERT_EQUAL_HEX32(0x123456, command.color);
    TEST_ASSERT_EQUAL_UINT32(2500, command.durationMs);
}

void test_request_effect_carries_engine_reactivity_and_background_cache() {
    MatrixState state;
    MatrixCommand command;

    state.requestEffect(3, 850, 0x010203, 0x040506, 0x070809, 0, 1, 1, 125);

    TEST_ASSERT_TRUE(state.poll(command));
    TEST_ASSERT_EQUAL(static_cast<int>(CommandType::SHOW_EFFECT), static_cast<int>(command.type));
    TEST_ASSERT_EQUAL_UINT8(3, command.value8);
    TEST_ASSERT_EQUAL_UINT8(1, command.effectEngine);
    TEST_ASSERT_EQUAL_UINT32(850, command.effectSpeedMs);
    TEST_ASSERT_EQUAL_HEX32(0x010203, command.value32);
    TEST_ASSERT_EQUAL_HEX32(0x040506, command.value32_2);
    TEST_ASSERT_EQUAL_HEX32(0x070809, command.value32_3);
    TEST_ASSERT_EQUAL_UINT8(1, command.effectReactivityProvider);
    TEST_ASSERT_EQUAL_UINT8(125, command.effectReactivityGain);

    const auto bg = state.getBackgroundEffect();
    TEST_ASSERT_TRUE(bg.active);
    TEST_ASSERT_EQUAL_UINT8(3, bg.mode);
    TEST_ASSERT_EQUAL_UINT8(1, bg.engine);
    TEST_ASSERT_EQUAL_UINT8(1, bg.reactivityProvider);
    TEST_ASSERT_EQUAL_UINT8(125, bg.reactivityGain);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_request_text_preserves_payload_up_to_command_buffer_size);
    RUN_TEST(test_request_effect_carries_engine_reactivity_and_background_cache);
    return UNITY_END();
}
