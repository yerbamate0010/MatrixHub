#ifndef MatrixService_h
#define MatrixService_h

#include <Arduino.h>
#include "types/MatrixTypes.h"
#include "renderer/MatrixRenderer.h"
#include "core/MatrixState.h"

class MatrixService {
public:
    MatrixService();

    void init(uint8_t pin);
    void loop();

    /**
     * Show scrolling text on matrix
     * @param text Text to display
     * @param color 565 color (default white)
     * @param durationMs Duration before auto-clear (0 = permanent until next command)
     */
    void showText(const char* text, uint32_t color = 0xFFFFFF, uint32_t durationMs = 0);
    
    /**
     * Show icon on matrix (thread-safe, deferred execution)
     * @param icon Icon type to display
     * @param durationMs Duration before auto-clear (0 = permanent until next command)
     */
    void showIcon(IconType icon, uint32_t durationMs = 0);
    
    /**
     * @brief Clear the matrix
     * @param stopBackground If true, stops and disables any background effect. 
     *                       If false, clear foreground but allow background effect to resume if active.
     */
    void clear(bool stopBackground = true);
    /**
     * @brief Drop the cached idle/background effect without forcing an immediate clear.
     *
     * Layered callers must use this when effects are disabled in settings.
     * Clearing only the BACKGROUND layer is not enough: a later clear(false)
     * path can otherwise resurrect the stale cached effect from MatrixState.
     */
    void clearBackgroundEffect();
    void setBrightness(uint8_t brightness);
    void setThermalBrightnessLimit(uint8_t limit);
    void setRotation(uint8_t rotation);
    void setScrollSpeed(uint16_t ms);

    // Forces a solid color (Active Mode - blocks passive updates)
    void showSolidColor(uint32_t color);

    // Returns true if renderer needs tight loop (scrolling or effect)
    bool isActive() const;



    // Custom Icons
    void setCustomIcon(IconType type, const uint32_t* bitmap);

    /**
     * @brief Show a WS2812FX effect
     * @param mode Effect mode index (0-50+)
     * @param speed Speed in milliseconds per animation step
     * @param color RGB888 Color (0xRRGGBB)
     * @param durationMs Duration before auto-clear (0 = permanent)
     */
    void showEffect(uint8_t mode, uint32_t speed, uint32_t color, uint32_t color2, uint32_t color3, uint32_t durationMs = 0);



private:

    // Renderer (Handles hardware & logic)
    MatrixRenderer _renderer;
    
    // State Container
    MatrixState _state;
    
    // Display State Cache (for optimization)
    IconType _activeIcon = IconType::NONE;

    // Auto-clear timing
    uint32_t _displayStartMs = 0;
    uint32_t _displayDurationMs = 0;
    bool _autoClearing = false;
};

#endif
