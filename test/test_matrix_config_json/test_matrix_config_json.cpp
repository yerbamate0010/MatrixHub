#include <unity.h>
#include <ArduinoJson.h>

#include "../../src/matrix/MatrixCustomIconStore.h"
#include "../../src/system/rtc/RtcConfig.h"

namespace RTC {
static ConfigStore mockStore;

const ConfigStore& getConfig() {
    return mockStore;
}

ConfigStore& getMutableConfig() {
    return mockStore;
}

void withConfig(const std::function<void(const ConfigStore&)>& reader) {
    reader(mockStore);
}

bool updateConfig(const std::function<void(ConfigStore&)>& updater) {
    updater(mockStore);
    return true;
}
}  // namespace RTC

namespace MATRIX {
static bool customIconHas[kMatrixCustomIconCount]{};
static uint32_t customIcons[kMatrixCustomIconCount][kMatrixCustomIconPixels]{};

bool hasCustomIcon(size_t index) {
    return index < kMatrixCustomIconCount && customIconHas[index];
}

bool copyCustomIcon(size_t index, uint32_t* outBuffer, size_t pixelCount) {
    if (index >= kMatrixCustomIconCount || !outBuffer || pixelCount != kMatrixCustomIconPixels ||
        !customIconHas[index]) {
        return false;
    }
    memcpy(outBuffer, customIcons[index], sizeof(customIcons[index]));
    return true;
}

bool setCustomIcon(size_t index, const uint32_t* bitmap, size_t pixelCount) {
    if (index >= kMatrixCustomIconCount || pixelCount != kMatrixCustomIconPixels) {
        return false;
    }
    if (!bitmap) {
        memset(customIcons[index], 0, sizeof(customIcons[index]));
        customIconHas[index] = false;
        return true;
    }
    for (size_t pixel = 0; pixel < kMatrixCustomIconPixels; ++pixel) {
        customIcons[index][pixel] = normalizeMatrixColor(bitmap[pixel]);
    }
    customIconHas[index] = true;
    return true;
}

bool clearCustomIcon(size_t index) {
    return setCustomIcon(index, nullptr);
}

void clearAllCustomIcons() {
    memset(customIcons, 0, sizeof(customIcons));
    memset(customIconHas, 0, sizeof(customIconHas));
}

bool customIconEquals(size_t index, const uint32_t* bitmap, size_t pixelCount) {
    if (index >= kMatrixCustomIconCount || pixelCount != kMatrixCustomIconPixels ||
        !customIconHas[index] || !bitmap) {
        return false;
    }
    for (size_t pixel = 0; pixel < kMatrixCustomIconPixels; ++pixel) {
        if (customIcons[index][pixel] != normalizeMatrixColor(bitmap[pixel])) {
            return false;
        }
    }
    return true;
}
}  // namespace MATRIX

#include "../../src/config/json/MatrixConfigJson.cpp"

void setUp(void) {
    RTC::mockStore = RTC::ConfigStore{};
    MATRIX::clearAllCustomIcons();
}

void tearDown(void) {}

void test_deserialize_matrix_normalizes_rgb888_colors() {
    JsonDocument doc;
    doc[CONFIG::Keys::kEffectColor] = 0x1ABCDEF;
    doc[CONFIG::Keys::kEffectColor2] = 0x1000000;
    doc[CONFIG::Keys::kEffectColor3] = 0xFFFFFFFF;
    doc[CONFIG::Keys::kMenuTextColor] = 0x1FFFFFF;

    RTC::MatrixData data{};
    JsonObject obj = doc.as<JsonObject>();
    CONFIG::JSON::deserializeMatrix(obj, data);

    TEST_ASSERT_EQUAL_HEX32(0xABCDEF, data.effectColor);
    TEST_ASSERT_EQUAL_HEX32(0x000000, data.effectColor2);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF, data.effectColor3);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF, data.menu.textColor);
}

void test_load_matrix_psram_normalizes_custom_icon_pixels() {
    JsonDocument doc;
    JsonArray icons = doc[CONFIG::Keys::kCustomIcons].to<JsonArray>();
    JsonArray icon = icons.add<JsonArray>();
    for (size_t pixel = 0; pixel < MATRIX::kMatrixCustomIconPixels; ++pixel) {
        icon.add(pixel == 0 ? 0x1ABCDEF : pixel);
    }

    JsonObject obj = doc.as<JsonObject>();
    CONFIG::JSON::loadMatrixPsram(obj);

    uint32_t out[MATRIX::kMatrixCustomIconPixels]{};
    TEST_ASSERT_TRUE(MATRIX::copyCustomIcon(0, out));
    TEST_ASSERT_EQUAL_HEX32(0xABCDEF, out[0]);
    TEST_ASSERT_EQUAL_HEX32(1, out[1]);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_deserialize_matrix_normalizes_rgb888_colors);
    RUN_TEST(test_load_matrix_psram_normalizes_custom_icon_pixels);
    return UNITY_END();
}
