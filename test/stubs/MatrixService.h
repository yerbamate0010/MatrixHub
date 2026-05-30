#pragma once

#include <Arduino.h>
#include "types/MatrixTypes.h"

class MatrixService {
public:
    MatrixService() = default;

    void init(uint8_t pin) {}
    void loop() {}
    void showIcon(IconType icon, uint32_t duration) {}
    void showText(const char* text, uint32_t color, uint32_t duration) {
        (void)duration;
        lastText = text ? text : "";
        lastTextColor = color;
        showTextCalls++;
    }
    void showEffect(uint8_t mode, uint16_t speed, uint32_t color1, uint32_t color2, uint32_t color3) {
        lastEffectMode = mode;
        lastEffectSpeed = speed;
        lastEffectColor1 = color1;
        lastEffectColor2 = color2;
        lastEffectColor3 = color3;
        showEffectCalls++;
    }
    void showSolidColor(uint16_t color) {}
    void clear(bool stopBackground = true) {
        lastClearStopBackground = stopBackground;
        clearCalls++;
    }
    void clearBackgroundEffect() { clearBackgroundEffectCalls++; }
    void setBrightness(uint8_t brightness) {}
    void setScrollSpeed(uint16_t speed) { lastScrollSpeed = speed; }
    uint8_t lastLimit = 255;
    void setThermalBrightnessLimit(uint8_t limit) { lastLimit = limit; }
    void setRotation(uint8_t rotation) {
        lastRotation = rotation;
        setRotationCalls++;
    }
    void setCustomIcon(IconType type, const uint32_t* bitmap) {
        const size_t index = static_cast<size_t>(type);
        if (index < 4) {
            customIconAssigned[index] = bitmap != nullptr;
        }
    }
    bool isActive() const { return false; }
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return 0; }

    const char* lastText = nullptr;
    uint32_t lastTextColor = 0;
    uint32_t showTextCalls = 0;
    uint32_t showEffectCalls = 0;
    uint32_t clearCalls = 0;
    uint32_t clearBackgroundEffectCalls = 0;
    bool lastClearStopBackground = false;
    uint8_t lastEffectMode = 0;
    uint16_t lastEffectSpeed = 0;
    uint32_t lastEffectColor1 = 0;
    uint32_t lastEffectColor2 = 0;
    uint32_t lastEffectColor3 = 0;
    uint16_t lastScrollSpeed = 0;
    uint8_t lastRotation = 0;
    uint32_t setRotationCalls = 0;
    bool customIconAssigned[4] = {false, false, false, false};
};
