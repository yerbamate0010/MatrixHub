#pragma once

#include <Arduino.h>
#include <cstdint>
#include <algorithm>

namespace UI {
    namespace BOOT {
        constexpr uint32_t WAIT_FOR_SERVICES_MS = 2000;
        constexpr uint32_t TASK_STARTUP_DELAY_MS = 20;
    }
    namespace MATRIX {
        constexpr uint8_t BRIGHTNESS_OFF = 0;
        constexpr uint8_t BRIGHTNESS_DEFAULT = 2; // Reduced from 10 for thermal safety
        constexpr uint32_t SCROLL_INTERVAL_MS = 20; // Default scroll speed (ms/px). Lower = faster
        constexpr uint32_t MENU_TEXT_COLOR_DEFAULT = 0xFFFFFF; // RGB888 white
        constexpr bool MENU_ENABLED_DEFAULT = true;
        constexpr int FONT_WIDTH_PX = 6;
        constexpr size_t TEXT_CAPACITY = 96;
        constexpr size_t TEXT_PADDING_CHARS = 4;
        constexpr size_t RENDERED_TEXT_CAPACITY = TEXT_CAPACITY + TEXT_PADDING_CHARS;
        constexpr uint8_t NOTIFICATION_QUEUE_CAPACITY = 8;
        
        // Menu refresh interval (updates content; dedup skips re-render if unchanged)
        constexpr uint32_t MENU_REFRESH_MS = 2000; // All screens refresh every 2s

        // Hardware Constants & Thresholds
        constexpr uint16_t LED_COUNT = 64; // 8x8 matrix
        
        // Default Settings
        constexpr int8_t DEFAULT_ROTATION = 0;
        constexpr bool DEFAULT_AUTO_ROTATE = false;
        constexpr bool DEFAULT_EFFECT_ENABLED = false;
        constexpr uint8_t DEFAULT_EFFECT_MODE = 0;      // Static
        constexpr uint32_t DEFAULT_EFFECT_SPEED = 1000;
        constexpr uint32_t MIN_EFFECT_SPEED = 50;
        constexpr uint32_t MAX_EFFECT_SPEED = 24UL * 60UL * 60UL * 1000UL; // 24h
        constexpr uint16_t EFFECT_DRIVER_SPEED_MAX = 65535;
        constexpr uint32_t DEFAULT_EFFECT_COLOR_PRIMARY = 0x00FF00;   // Green
        constexpr uint32_t DEFAULT_EFFECT_COLOR_SECONDARY = 0xFF0000; // Red
        constexpr uint32_t DEFAULT_EFFECT_COLOR_TERTIARY = 0x0000FF;  // Blue

        constexpr uint32_t AUTO_ROTATE_INTERVAL_MS = 500;
        constexpr float AUTO_ROTATE_THRESHOLD_G = 0.65f;
    }
    namespace BUTTON {
        constexpr uint32_t DOUBLE_CLICK_WINDOW_MS = 400;  // Max time between clicks for double-click
    }
}
