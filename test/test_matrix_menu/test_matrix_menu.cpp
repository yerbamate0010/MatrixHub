/**
 * @file test_matrix_menu.cpp
 * @brief Unit tests for MatrixMenuService behavior
 */

#ifdef UNIT_TEST

#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>

#include "../../src/system/rtc/RtcConfig.h"
#include "../../src/sensors/runtime/SensorState.h"
#include "../../src/system/logging/Logging.h"
#include "../../src/system/matrix_manager/MatrixManagerService.h"

// ---------------------------------------------------------------------------
// RTC stubs (avoid linking full RtcConfig.cpp)
// ---------------------------------------------------------------------------
namespace RTC {
    ConfigStore mockStore;

    ConfigStore& getMutableConfig() {
        return mockStore;
    }

    const ConfigStore& getConfig() {
        return mockStore;
    }

    void withConfig(const std::function<void(const ConfigStore&)>& reader) {
        reader(mockStore);
    }
}

// ---------------------------------------------------------------------------
// SensorState stub (avoid linking sensor runtime)
// ---------------------------------------------------------------------------
namespace SENSORS {
    SemaphoreHandle_t SensorState::_initMutex = nullptr;
    SemaphoreHandle_t SensorState::_snapshotMutex = nullptr;
    SensorSnapshot SensorState::_latestSnapshot{};
    SensorSnapshot SensorState::_lastGoodSnapshot{};
    PhaseStatus SensorState::_lastReadStatus{};
    PhaseStatus SensorState::_lastWriteStatus{};
    ErrorInfo SensorState::_lastErrorInfo{};
    std::atomic<bool> SensorState::_initialized{false};

    SensorSnapshot SensorState::getSnapshot() {
        SensorSnapshot snap{};
        snap.timestamp_ms = 0;
        snap.temp = 0.0f;
        snap.humid = 0.0f;
        snap.co2 = 0;
        return snap;
    }
}

// ---------------------------------------------------------------------------
// Logging stubs
// ---------------------------------------------------------------------------
namespace LOG {
    Settings Logging::_settings;
    void Logging::log(esp_log_level_t level, const char* tag, const char* format, ...) {}
    void Logging::logSection(const char* title) {}
    void Logging::logStackHwm(const char* taskName, uint32_t stackSize) {}
}

// ---------------------------------------------------------------------------
// Time control for MatrixMenuService.cpp
// ---------------------------------------------------------------------------
static unsigned long _mockMillis = 0;
#define millis() _mockMillis
#include "../../src/matrix/menu/MatrixMenuService.cpp"
#undef millis

// ---------------------------------------------------------------------------
// MatrixManagerService stubs (linker)
// ---------------------------------------------------------------------------
namespace MATRIX_MANAGER {
    static uint32_t g_setLayerCalls = 0;
    static uint32_t g_clearLayerCalls = 0;
    static Layer g_lastSetLayer = Layer::BACKGROUND;
    static Layer g_lastClearLayer = Layer::BACKGROUND;

    void MatrixManagerService::setLayer(Layer layer, const LayerContent& content) {
        g_setLayerCalls++;
        g_lastSetLayer = layer;
        (void)content;
    }

    void MatrixManagerService::clearLayer(Layer layer) {
        g_clearLayerCalls++;
        g_lastClearLayer = layer;
    }
}

// WiFi global from stub
WiFiClass WiFi;

using namespace MATRIX;

static MatrixService g_matrix;

void setUp(void) {
    memset(&RTC::mockStore, 0, sizeof(RTC::mockStore));
    RTC::mockStore.matrix.menu.enabled = true;
    RTC::mockStore.matrix.menu.textColor = 0xFFFFFF;
    RTC::mockStore.matrix.menu.scrollSpeed = 20;
    _mockMillis = 0;

    g_matrix.showTextCalls = 0;
    g_matrix.clearCalls = 0;
    MATRIX_MANAGER::g_setLayerCalls = 0;
    MATRIX_MANAGER::g_clearLayerCalls = 0;
    MATRIX_MANAGER::g_lastSetLayer = MATRIX_MANAGER::Layer::BACKGROUND;
    MATRIX_MANAGER::g_lastClearLayer = MATRIX_MANAGER::Layer::BACKGROUND;

}

void tearDown(void) {}

void test_menu_cycles_screens() {
    MatrixMenuService menu(&g_matrix, nullptr);
    menu.nextScreen();
    TEST_ASSERT_TRUE(menu.isActive());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::TIME),
                            static_cast<uint8_t>(menu.current()));

    menu.nextScreen();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::SENSORS),
                            static_cast<uint8_t>(menu.current()));

    menu.nextScreen();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::IP),
                            static_cast<uint8_t>(menu.current()));

    menu.nextScreen();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::TIME),
                            static_cast<uint8_t>(menu.current()));
}

void test_enter_menu_opens_time_screen_without_advancing_active_menu() {
    MatrixMenuService menu(&g_matrix, nullptr);
    menu.enterMenu();
    TEST_ASSERT_TRUE(menu.isActive());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::TIME),
                            static_cast<uint8_t>(menu.current()));

    menu.enterMenu();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::TIME),
                            static_cast<uint8_t>(menu.current()));
}

void test_menu_disables_and_exits() {
    MatrixMenuService menu(&g_matrix, nullptr);
    menu.nextScreen();
    TEST_ASSERT_TRUE(menu.isActive());

    RTC::mockStore.matrix.menu.enabled = false;
    menu.update();

    TEST_ASSERT_FALSE(menu.isActive());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RTC::MatrixMenuScreen::NONE),
                            static_cast<uint8_t>(menu.current()));
    TEST_ASSERT_TRUE(g_matrix.clearCalls > 0);
}

void test_exit_menu_clears_menu_layer_when_manager_present() {
    alignas(MATRIX_MANAGER::MatrixManagerService) unsigned char managerStorage[sizeof(MATRIX_MANAGER::MatrixManagerService)] = {};
    auto* manager = reinterpret_cast<MATRIX_MANAGER::MatrixManagerService*>(managerStorage);

    MatrixMenuService menu(&g_matrix, manager);
    menu.enterMenu();
    TEST_ASSERT_EQUAL_UINT32(1, MATRIX_MANAGER::g_setLayerCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MATRIX_MANAGER::Layer::MENU),
                            static_cast<uint8_t>(MATRIX_MANAGER::g_lastSetLayer));

    menu.exitMenu();
    TEST_ASSERT_EQUAL_UINT32(1, MATRIX_MANAGER::g_clearLayerCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MATRIX_MANAGER::Layer::MENU),
                            static_cast<uint8_t>(MATRIX_MANAGER::g_lastClearLayer));
}

void test_invalidate_cache_forces_rerender() {
    MatrixMenuService menu(&g_matrix, nullptr);
    menu.nextScreen();
    TEST_ASSERT_EQUAL_UINT32(1, g_matrix.showTextCalls);

    // First update syncs internal timers/state
    _mockMillis = 10;
    menu.update();
    TEST_ASSERT_EQUAL_UINT32(1, g_matrix.showTextCalls);

    // Cache invalidation should force a new render even if refresh window not reached
    _mockMillis = 100;
    menu.invalidateCache();
    menu.update();
    TEST_ASSERT_EQUAL_UINT32(2, g_matrix.showTextCalls);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_menu_cycles_screens);
    RUN_TEST(test_enter_menu_opens_time_screen_without_advancing_active_menu);
    RUN_TEST(test_menu_disables_and_exits);
    RUN_TEST(test_exit_menu_clears_menu_layer_when_manager_present);
    RUN_TEST(test_invalidate_cache_forces_rerender);
    return UNITY_END();
}

#endif
