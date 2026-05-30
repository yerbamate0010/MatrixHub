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

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_request_text_preserves_payload_up_to_command_buffer_size);
    return UNITY_END();
}
