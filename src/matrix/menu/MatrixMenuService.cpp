/**
 * @file MatrixMenuService.cpp
 * @brief Button-controlled menu for Matrix LED display
 * 
 * Golden Standard Implementation:
 * - Lock-free isActive() for button responsiveness
 * - Minimal mutex hold time (state only, no rendering under lock)
 * - Stack-allocated fixed buffers (no heap/DRAM allocation)
 * - Configurable refresh intervals via System.h
 */

#include "MatrixMenuService.h"
#include <MatrixService.h>
#include "../../system/matrix_manager/MatrixManagerService.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>
#include "../../sensors/runtime/SensorState.h"
#include "../../system/rtc/RtcConfig.h"
#include "../../system/logging/Logging.h"
#include "../../config/System.h"
#include "../../system/utils/ScopeLock.h"

#undef LOG_TAG
#define LOG_TAG "MatrixMenu"

namespace MATRIX {

// Static buffer for text rendering (avoids heap allocation)
// Max: "https://long-hostname.local https://192.168.100.100" = ~60 chars
static constexpr size_t kTextBufferSize = kMatrixTextCapacity;

MatrixMenuService::MatrixMenuService(MatrixService* matrixService, MATRIX_MANAGER::MatrixManagerService* matrixManager)
    : _matrixService(matrixService), _matrixManager(matrixManager) {
    configASSERT(matrixService != nullptr);
    _mutex = xSemaphoreCreateMutexStatic(&_mutexBuffer);
    // Sync atomic flag with actual state (safety for warm restarts)
    _isActive.store(_screen.load(std::memory_order_relaxed) != RTC::MatrixMenuScreen::NONE, std::memory_order_relaxed);
}

bool MatrixMenuService::isEnabled() const {
    return RTC::getConfig().matrix.menu.enabled;
}

bool MatrixMenuService::isActive() const {
    // Lock-free check for high-performance loops (ButtonHandler)
    return _isActive.load(std::memory_order_relaxed);
}

RTC::MatrixMenuScreen MatrixMenuService::current() const {
    return _screen.load(std::memory_order_relaxed);
}

void MatrixMenuService::enterMenu() {
    if (isActive()) {
        return;
    }
    nextScreen();
}

void MatrixMenuService::nextScreen() {
    if (!_mutex) return;
    if (!isEnabled()) {
        exitMenu();
        return;
    }
    
    RTC::MatrixMenuScreen screenToRender = RTC::MatrixMenuScreen::NONE;
    
    {
        SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(100));
        if (!lock.isLocked()) {
            LOGE("Failed to take mutex for nextScreen");
            return;
        }
        
        const auto cur = _screen.load(std::memory_order_relaxed);
        RTC::MatrixMenuScreen next = RTC::MatrixMenuScreen::TIME;
        switch (cur) {
            case RTC::MatrixMenuScreen::NONE:    next = RTC::MatrixMenuScreen::TIME;    break;
            case RTC::MatrixMenuScreen::TIME:    next = RTC::MatrixMenuScreen::SENSORS; break;
            case RTC::MatrixMenuScreen::SENSORS: next = RTC::MatrixMenuScreen::IP;      break;
            case RTC::MatrixMenuScreen::IP:
            default:                             next = RTC::MatrixMenuScreen::TIME;    break;
        }
        
        _screen.store(next, std::memory_order_relaxed);
        _isActive.store(true, std::memory_order_relaxed);
        screenToRender = next;
        _lastRenderedScreen = RTC::MatrixMenuScreen::NONE;
        _lastRenderedText[0] = '\0'; // Force re-render on screen change
        LOGI("Menu screen: %u", static_cast<unsigned>(next));
    } // mutex released here
    
    (void)renderScreen(screenToRender, true);
}

void MatrixMenuService::exitMenu() {
    if (!_mutex) return;
    
    bool wasActive = false;
    {
        SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(100));
        if (!lock.isLocked()) {
            LOGE("Failed to take mutex for exitMenu");
            return;
        }
        if (_screen.load(std::memory_order_relaxed) != RTC::MatrixMenuScreen::NONE) {
            _screen.store(RTC::MatrixMenuScreen::NONE, std::memory_order_relaxed);
            _lastRenderedScreen = RTC::MatrixMenuScreen::NONE;
            _isActive.store(false, std::memory_order_relaxed);
            wasActive = true;
            LOGI("Menu force closed");
        }
    } // mutex released here
    
    if (wasActive) {
        // Clear MENU layer so Manager knows menu is no longer active
        // Manager will automatically re-render the next highest layer (like ALARM)
        if (_matrixManager) _matrixManager->clearLayer(MATRIX_MANAGER::Layer::MENU);
        else _matrixService->clear(false); // Fallback for testing/native
    }
}

void MatrixMenuService::invalidateCache() {
    if (!_isActive.load(std::memory_order_relaxed)) return;
    if (!_mutex) return;
    SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(10));
    if (!lock.isLocked()) return;
    _lastUpdateMs = 0;           // Force timer expiry on next update()
    _lastRenderedScreen = RTC::MatrixMenuScreen::NONE; // Force immediate re-render
    _lastRenderedText[0] = '\0'; // Clear dedup so text is re-sent
}

void MatrixMenuService::update() {
    if (!_isActive.load(std::memory_order_relaxed)) return;
    if (!_mutex) return;
    if (!isEnabled()) {
        exitMenu();
        return;
    }
    
    RTC::MatrixMenuScreen screenToRender = RTC::MatrixMenuScreen::NONE;
    bool forceRender = false;
    {
        SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(10));
        if (!lock.isLocked()) return; // Skip this frame if busy
        
        const auto cur = _screen.load(std::memory_order_relaxed);
        if (cur == RTC::MatrixMenuScreen::NONE) return;
        
        const uint32_t now = millis();
        if (cur != _lastRenderedScreen || (now - _lastUpdateMs) >= UI::MATRIX::MENU_REFRESH_MS) {
            screenToRender = cur;
            forceRender = (cur != _lastRenderedScreen);
        }
    } // mutex released here
    
    if (screenToRender != RTC::MatrixMenuScreen::NONE) {
        (void)renderScreen(screenToRender, forceRender);
    }
}

uint32_t MatrixMenuService::estimateScrollDurationMs(const char* text, uint16_t scrollSpeedMs) const {
    if (!text || scrollSpeedMs == 0) return 0;
    const size_t len = strnlen(text, kTextBufferSize - 1);
    constexpr uint32_t kMatrixWidthPx = 8; // 8x8 matrix
    const uint32_t perCharMs = UI::MATRIX::FONT_WIDTH_PX * scrollSpeedMs;
    const uint32_t paddedLen = static_cast<uint32_t>(len) + 4; // MatrixRenderer adds 2 spaces front/back
    return (kMatrixWidthPx * scrollSpeedMs) + (paddedLen * perCharMs);
}

bool MatrixMenuService::renderScreen(RTC::MatrixMenuScreen screen, bool forceRender) {
    const auto& settings = RTC::getConfig().matrix.menu;
    if (!settings.enabled) {
        return false;
    }
    
    // Fixed-size stack buffer (no heap allocation)
    char textBuf[kTextBufferSize];
    
    switch (screen) {
        case RTC::MatrixMenuScreen::TIME:
            formatTime(textBuf, sizeof(textBuf));
            break;
        case RTC::MatrixMenuScreen::SENSORS:
            formatSensors(textBuf, sizeof(textBuf));
            break;
        case RTC::MatrixMenuScreen::IP:
            formatIP(textBuf, sizeof(textBuf));
            break;
        default:
            return false;
    }

    uint16_t scrollSpeedMs = settings.scrollSpeed;
    if (scrollSpeedMs < 20 || scrollSpeedMs > 120) {
        scrollSpeedMs = UI::MATRIX::SCROLL_INTERVAL_MS;
    }

    // Validate against the last committed render state first, but do not update
    // the dedup cache yet. If the actual render path aborts, we want the next
    // update() tick to retry instead of believing this frame was already shown.
    const uint32_t now = millis();
    if (_mutex) {
        SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(10));
        if (!lock.isLocked()) return false; // Skip this frame if cache is busy

        if (!forceRender && _scrollHoldUntilMs != 0 && (int32_t)(now - _scrollHoldUntilMs) < 0) {
            return false; // Keep current scroll running to completion
        }

        if (!forceRender && strcmp(textBuf, _lastRenderedText) == 0) {
            LOGD("Text unchanged, skipping render");
            return false;
        }
    } else {
        if (!forceRender && _scrollHoldUntilMs != 0 && (int32_t)(now - _scrollHoldUntilMs) < 0) {
            return false;
        }
        if (!forceRender && strcmp(textBuf, _lastRenderedText) == 0) {
            LOGD("Text unchanged, skipping render");
            return false;
        }
    }
    
    if (!_isActive.load(std::memory_order_relaxed) ||
        _screen.load(std::memory_order_relaxed) != screen) {
        return false;
    }
    if (_matrixManager) {
        // Render via manager to respect layer priority (no direct renderer writes)
        MATRIX_MANAGER::LayerContent layerContent;
        layerContent.active = true;
        layerContent.type = CommandType::SHOW_TEXT;
        strlcpy(layerContent.text, textBuf, sizeof(layerContent.text));
        layerContent.color = settings.textColor;
        _matrixManager->setLayer(MATRIX_MANAGER::Layer::MENU, layerContent);
    } else {
        // Fallback for tests/native or when manager is not present
        _matrixService->showText(textBuf, settings.textColor, 0);
    }

    const uint32_t scrollHoldUntilMs = now + estimateScrollDurationMs(textBuf, scrollSpeedMs);
    if (_mutex) {
        SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(10));
        if (lock.isLocked()) {
            strlcpy(_lastRenderedText, textBuf, sizeof(_lastRenderedText));
            _scrollHoldUntilMs = scrollHoldUntilMs;
            _lastRenderedScreen = screen;
            _lastUpdateMs = now;
        }
    } else {
        strlcpy(_lastRenderedText, textBuf, sizeof(_lastRenderedText));
        _scrollHoldUntilMs = scrollHoldUntilMs;
        _lastRenderedScreen = screen;
        _lastUpdateMs = now;
    }
    
    LOGD("Rendered: %s", textBuf);
    return true;
}

void MatrixMenuService::formatTime(char* buf, size_t bufSize) {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    snprintf(buf, bufSize, "%02d.%02d %02d:%02d",
             timeinfo.tm_mday,
             timeinfo.tm_mon + 1,
             timeinfo.tm_hour,
             timeinfo.tm_min);
}

void MatrixMenuService::formatSensors(char* buf, size_t bufSize) {
    auto snapshot = SENSORS::SensorState::getSnapshot();
    
    if (snapshot.timestamp_ms > 0) {
        // Optimization: cast floats to int to avoid expensive float string formatting (%.0f)
        snprintf(buf, bufSize, "T:%d H:%d C:%u",
                 (int)(snapshot.temp + 0.5f),
                 (int)(snapshot.humid + 0.5f),
                 snapshot.co2);
    } else {
        snprintf(buf, bufSize, "T:-- H:-- C:--");
    }
}

void MatrixMenuService::formatIP(char* buf, size_t bufSize) {
    wifi_mode_t mode = WiFi.getMode();
    
    // Zero-alloc IP formatting (avoid Arduino String heap allocation)
    char ipBuf[16] = {0};
    
    if (mode == WIFI_STA || mode == WIFI_AP_STA) {
        if (WiFi.status() == WL_CONNECTED) {
            uint32_t ip = WiFi.localIP();
            snprintf(ipBuf, sizeof(ipBuf), "%u.%u.%u.%u", 
                     (ip & 0xFF), ((ip >> 8) & 0xFF), ((ip >> 16) & 0xFF), (ip >> 24));
        }
    }
    if (ipBuf[0] == '\0' && (mode == WIFI_AP || mode == WIFI_AP_STA)) {
        uint32_t ip = WiFi.softAPIP();
        snprintf(ipBuf, sizeof(ipBuf), "%u.%u.%u.%u", 
                 (ip & 0xFF), ((ip >> 8) & 0xFF), ((ip >> 16) & 0xFF), (ip >> 24));
    }
    
    if (ipBuf[0] == '\0' || strcmp(ipBuf, "0.0.0.0") == 0) {
        strncpy(buf, "No WiFi", bufSize - 1);
        buf[bufSize - 1] = '\0';
        return;
    }
    
    const char* hostname = WiFi.getHostname();
    if (hostname && hostname[0] != '\0') {
        snprintf(buf, bufSize, "https://%s.local https://%s", hostname, ipBuf);
    } else {
        snprintf(buf, bufSize, "https://%s", ipBuf);
    }
}

}  // namespace MATRIX
