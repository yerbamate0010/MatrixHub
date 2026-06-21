/**
 * @file MatrixConfigJson.cpp
 * @brief JSON serialization for Matrix LED configuration
 */

#include "MatrixConfigJson.h"
#include "../App.h"
#include "../UiConfig.h"
#include "../../matrix/MatrixCustomIconStore.h"
#include "../../matrix/MatrixEffectModes.h"
#include "../../system/rtc/RtcConfig.h"
#include <algorithm>

namespace {

bool loadIconFromJson(uint8_t iconIndex, JsonVariant value) {
    if (!value.is<JsonArray>()) {
        return false;
    }

    JsonArray iconData = value.as<JsonArray>();
    if (iconData.size() == MATRIX::kMatrixCustomIconPixels) {
        uint32_t bitmap[MATRIX::kMatrixCustomIconPixels];
        for (size_t pixel = 0; pixel < MATRIX::kMatrixCustomIconPixels; pixel++) {
            bitmap[pixel] = MATRIX::normalizeMatrixColor(iconData[pixel].as<uint32_t>());
        }
        MATRIX::setCustomIcon(iconIndex, bitmap);
        return true;
    }

    if (iconData.size() == 0) {
        MATRIX::clearCustomIcon(iconIndex);
    }

    return true;
}

}  // namespace

namespace CONFIG {
namespace JSON {

void deserializeMatrix(JsonObject& obj, RTC::MatrixData& data) {
    // Brightness (0-255)
    if (obj[Keys::kBrightness].is<uint8_t>()) {
        uint8_t v = obj[Keys::kBrightness].as<uint8_t>();
        data.brightness = (uint8_t)std::clamp<int>(v, 0, 255);
    }

    // Alarm mode (0=SOLID, 1=ICON, 2=SCROLL_TEXT)
    if (obj[Keys::kAlarmMode].is<uint8_t>()) {
        uint8_t v = obj[Keys::kAlarmMode].as<uint8_t>();
        if (v > 2) v = 0;
        data.alarmMode = static_cast<RTC::MatrixAlarmMode>(v);
    }

    // Rotation (0-3)
    if (obj[Keys::kRotation].is<uint8_t>()) {
        uint8_t v = obj[Keys::kRotation].as<uint8_t>();
        if (v > 3) v = 0;
        data.rotation = v;
    }

    // Auto Rotate
    if (obj[Keys::kAutoRotate].is<bool>()) {
        data.autoRotate = obj[Keys::kAutoRotate].as<bool>();
    }

    // Effects
    if (obj[Keys::kEffectEnabled].is<bool>()) {
        data.effectEnabled = obj[Keys::kEffectEnabled].as<bool>();
    }
    if (obj[Keys::kEffectEngine].is<uint8_t>()) {
        data.effectEngine = MATRIX::normalizeMatrixEffectEngine(obj[Keys::kEffectEngine].as<uint8_t>());
    }
    if (obj[Keys::kEffectMode].is<uint8_t>()) {
        // Reject hidden / unsupported IDs early so a crafted JSON payload
        // cannot enable matrix modes that the runtime does not fully support.
        data.effectMode = MATRIX::normalizeMatrixEffectMode(obj[Keys::kEffectMode].as<uint8_t>());
    }
    if (obj[Keys::kEffectSpeed].is<uint32_t>()) {
        uint32_t v = obj[Keys::kEffectSpeed].as<uint32_t>();
        data.effectSpeed = std::clamp<uint32_t>(
            v,
            UI::MATRIX::MIN_EFFECT_SPEED,
            UI::MATRIX::MAX_EFFECT_SPEED);
    }
    if (obj[Keys::kEffectColor].is<uint32_t>()) {
        data.effectColor = MATRIX::normalizeMatrixColor(obj[Keys::kEffectColor].as<uint32_t>());
    }
    if (obj[Keys::kEffectColor2].is<uint32_t>()) {
        data.effectColor2 = MATRIX::normalizeMatrixColor(obj[Keys::kEffectColor2].as<uint32_t>());
    }
    if (obj[Keys::kEffectColor3].is<uint32_t>()) {
        data.effectColor3 = MATRIX::normalizeMatrixColor(obj[Keys::kEffectColor3].as<uint32_t>());
    }
    if (obj[Keys::kEffectReactivityProvider].is<uint8_t>()) {
        data.effectReactivityProvider = MATRIX::normalizeMatrixEffectReactivityProvider(
            obj[Keys::kEffectReactivityProvider].as<uint8_t>());
    }
    if (obj[Keys::kEffectReactivityGain].is<uint8_t>()) {
        data.effectReactivityGain = MATRIX::normalizeMatrixEffectReactivityGain(
            obj[Keys::kEffectReactivityGain].as<uint8_t>());
    }
    data.effectMode = MATRIX::normalizeMatrixEffectModeForEngine(data.effectMode, data.effectEngine);

    // Menu settings
    if (obj[Keys::kMenuTextColor].is<uint32_t>()) {
        data.menu.textColor = MATRIX::normalizeMatrixColor(obj[Keys::kMenuTextColor].as<uint32_t>());
    }
    if (obj[Keys::kMenuScrollSpeed].is<uint16_t>()) {
        uint16_t v = obj[Keys::kMenuScrollSpeed].as<uint16_t>();
        data.menu.scrollSpeed = (uint16_t)std::clamp<int>(v, 20, 120);
    }
    if (obj[Keys::kMenuEnabled].is<bool>()) {
        data.menu.enabled = obj[Keys::kMenuEnabled].as<bool>();
    }
}

void serializeMatrix(const RTC::MatrixData& data, JsonObject& obj) {
    obj[Keys::kBrightness] = data.brightness;
    obj[Keys::kAlarmMode] = static_cast<uint8_t>(data.alarmMode);
    obj[Keys::kRotation] = data.rotation;
    obj[Keys::kAutoRotate] = data.autoRotate;

    // Effects
    obj[Keys::kEffectEnabled] = data.effectEnabled;
    obj[Keys::kEffectEngine] = data.effectEngine;
    obj[Keys::kEffectMode] = data.effectMode;
    obj[Keys::kEffectSpeed] = data.effectSpeed;
    obj[Keys::kEffectColor] = data.effectColor;
    obj[Keys::kEffectColor2] = data.effectColor2;
    obj[Keys::kEffectColor3] = data.effectColor3;
    obj[Keys::kEffectReactivityProvider] = data.effectReactivityProvider;
    obj[Keys::kEffectReactivityGain] = data.effectReactivityGain;

    // Menu settings
    obj[Keys::kMenuTextColor] = data.menu.textColor;
    obj[Keys::kMenuScrollSpeed] = data.menu.scrollSpeed;
    obj[Keys::kMenuEnabled] = data.menu.enabled;
}

void loadMatrixPsram(JsonObject& obj) {
    if (!obj[Keys::kCustomIcons].is<JsonArray>()) {
        return;
    }

    JsonArray icons = obj[Keys::kCustomIcons].as<JsonArray>();
    for (size_t i = 0; i < icons.size() && i < MATRIX::kMatrixCustomIconCount; i++) {
        loadIconFromJson(i, icons[i]);
    }
}

bool matrixCustomIconsChanged(JsonObject& obj) {
    if (!obj[Keys::kCustomIcons].is<JsonArray>()) {
        return false;
    }

    JsonArray icons = obj[Keys::kCustomIcons].as<JsonArray>();
    for (size_t i = 0; i < icons.size() && i < MATRIX::kMatrixCustomIconCount; i++) {
        if (!icons[i].is<JsonArray>()) {
            continue;
        }

        JsonArray iconData = icons[i].as<JsonArray>();
        if (iconData.size() == 0) {
            if (MATRIX::hasCustomIcon(i)) {
                return true;
            }
            continue;
        }

        if (iconData.size() != MATRIX::kMatrixCustomIconPixels) {
            continue;
        }

        uint32_t bitmap[MATRIX::kMatrixCustomIconPixels];
        for (size_t pixel = 0; pixel < MATRIX::kMatrixCustomIconPixels; pixel++) {
            bitmap[pixel] = MATRIX::normalizeMatrixColor(iconData[pixel].as<uint32_t>());
        }

        if (!MATRIX::hasCustomIcon(i) || !MATRIX::customIconEquals(i, bitmap)) {
            return true;
        }
    }

    return false;
}

void loadMatrix(JsonObject& obj) {
    RTC::updateConfigSection(&RTC::ConfigStore::matrix, [&](RTC::MatrixData& matrix) {
        deserializeMatrix(obj, matrix);
    });
    loadMatrixPsram(obj);
}

void saveMatrix(JsonObject& obj) {
    RTC::MatrixData currentCfg{};
    RTC::withConfig([&](const RTC::ConfigStore& cfg) {
        currentCfg = cfg.matrix;
    });
    serializeMatrix(currentCfg, obj);

    JsonArray icons = obj[Keys::kCustomIcons].to<JsonArray>();
    for (size_t i = 0; i < MATRIX::kMatrixCustomIconCount; i++) {
        JsonArray icon = icons.add<JsonArray>();
        uint32_t bitmap[MATRIX::kMatrixCustomIconPixels];
        if (MATRIX::copyCustomIcon(i, bitmap)) {
            for (size_t pixel = 0; pixel < MATRIX::kMatrixCustomIconPixels; pixel++) {
                icon.add(bitmap[pixel]);
            }
        }
    }
}

} // namespace JSON
} // namespace CONFIG
