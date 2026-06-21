#ifdef UNIT_TEST

#include <unity.h>

#define private public
#include "../../src/system/matrix/MatrixTask.h"
#undef private

#include "../../src/system/logging/Logging.h"
#include "../../src/system/rtc/RtcConfig.h"
#include "../../src/system/watchdog/TaskWatchdog.h"
#include "../../src/sensors/imu/ImuManager.h"
#include "../../src/sensors/imu/ImuService.h"
#include "../../test/stubs/esp_heap_caps.h"

namespace RTC {

ConfigStore mockStore{};

const ConfigStore& getConfig() {
    return mockStore;
}

ConfigStore& getMutableConfig() {
    return mockStore;
}

void withConfig(const std::function<void(const ConfigStore&)>& reader) {
    reader(mockStore);
}

}  // namespace RTC

namespace LOG {

Settings Logging::_settings{ESP_LOG_INFO};

void Logging::begin(const Settings& settings) { _settings = settings; }
void Logging::setLevel(esp_log_level_t level) { _settings.level = level; }
Settings Logging::settings() { return _settings; }
bool Logging::isEnabled(esp_log_level_t level) {
    return level <= _settings.level;
}
void Logging::log(esp_log_level_t level, const char* tag, const char* fmt, ...) {
    (void)level;
    (void)tag;
    (void)fmt;
}
void Logging::clearBuffer() {}
const char* Logging::levelToString(esp_log_level_t level) {
    (void)level;
    return "info";
}
esp_log_level_t Logging::stringToLevel(std::string_view name, esp_log_level_t fallback) {
    (void)name;
    return fallback;
}
void Logging::logStackHwm(const char* taskName, uint32_t stackSize) {
    (void)taskName;
    (void)stackSize;
}
void Logging::logSection(const char* title) {
    (void)title;
}
void Logging::suppressNoisyModules() {}

}  // namespace LOG

namespace SYSTEM {

void TaskWatchdog::begin(uint32_t timeoutSec, bool panicOnTimeout) {
    (void)panicOnTimeout;
    _timeoutSec = timeoutSec;
    _initialized = true;
}

bool TaskWatchdog::registerCurrentTask() { return true; }
bool TaskWatchdog::registerTask(TaskHandle_t taskHandle) {
    (void)taskHandle;
    return true;
}
bool TaskWatchdog::unregisterCurrentTask() { return true; }
bool TaskWatchdog::unregisterTask(TaskHandle_t taskHandle) {
    (void)taskHandle;
    return true;
}
bool TaskWatchdog::reset() { return true; }

}  // namespace SYSTEM

namespace {

bool g_hasAccel = false;
float g_ax = 0.0f;
float g_ay = 0.0f;
float g_az = 0.0f;
bool g_hasSample = false;
IMU::ImuSample g_sample{};
uint32_t g_setConsumerCalls = 0;
bool g_lastConsumerActive = false;
IMU::Consumer g_lastConsumer = IMU::Consumer::AutoRotate;

}  // namespace

bool ImuService::getCachedAccel(float& x, float& y, float& z) const {
    x = g_ax;
    y = g_ay;
    z = g_az;
    return g_hasAccel;
}

bool ImuService::getCachedSample(float& ax, float& ay, float& az, float& gx, float& gy, float& gz) const {
    ax = g_sample.ax;
    ay = g_sample.ay;
    az = g_sample.az;
    gx = g_sample.gx;
    gy = g_sample.gy;
    gz = g_sample.gz;
    return g_hasSample;
}

bool ImuService::getCachedSample(IMU::ImuSample& sample) const {
    sample = g_sample;
    return g_hasSample;
}

namespace IMU {

ImuManager::ImuManager(ImuService* imuService)
    : _imuService(imuService) {}

ImuManager::~ImuManager() = default;

void ImuManager::setConsumerActive(Consumer consumer, bool active) {
    g_lastConsumer = consumer;
    g_lastConsumerActive = active;
    g_setConsumerCalls++;
}

}  // namespace IMU

#include "../../src/system/matrix/MatrixTask.cpp"

namespace MATRIX {

void MatrixMenuService::update() {}

}  // namespace MATRIX

namespace MATRIX_MANAGER {

void MatrixManagerService::update() {}

}  // namespace MATRIX_MANAGER

namespace {

void resetState() {
    memset(&RTC::mockStore, 0, sizeof(RTC::mockStore));
    TEST_STUBS::ARDUINO::millisValue = 0;
    g_hasAccel = false;
    g_ax = 0.0f;
    g_ay = 0.0f;
    g_az = 0.0f;
    g_hasSample = false;
    g_sample = IMU::ImuSample{};
    g_setConsumerCalls = 0;
    g_lastConsumerActive = false;
    g_lastConsumer = IMU::Consumer::AutoRotate;
    MATRIX::MatrixTask::resetAutoRotationState();
}

}  // namespace

void setUp() {
    resetState();
}

void tearDown() {}

void test_auto_rotate_reapplies_rotation_after_toggle() {
    MatrixService matrix;
    ImuService imu;
    IMU::ImuManager manager(&imu);

    RTC::mockStore.matrix.autoRotate = true;
    RTC::mockStore.matrix.rotation = 0;
    g_hasAccel = true;
    g_ax = 1.0f;
    g_ay = 0.0f;
    g_az = 0.0f;

    TEST_STUBS::ARDUINO::millisValue = 100;
    MATRIX::MatrixTask::evaluateAutoRotation(&imu, &manager, &matrix);

    TEST_ASSERT_EQUAL_UINT32(1, matrix.setRotationCalls);
    TEST_ASSERT_EQUAL_UINT8(1, matrix.lastRotation);
    TEST_ASSERT_EQUAL_UINT32(1, g_setConsumerCalls);
    TEST_ASSERT_EQUAL(IMU::Consumer::AutoRotate, g_lastConsumer);
    TEST_ASSERT_TRUE(g_lastConsumerActive);

    RTC::mockStore.matrix.autoRotate = false;
    TEST_STUBS::ARDUINO::millisValue = 200;
    MATRIX::MatrixTask::evaluateAutoRotation(&imu, &manager, &matrix);

    TEST_ASSERT_EQUAL_UINT32(2, g_setConsumerCalls);
    TEST_ASSERT_FALSE(g_lastConsumerActive);

    RTC::mockStore.matrix.autoRotate = true;
    TEST_STUBS::ARDUINO::millisValue = 210;
    MATRIX::MatrixTask::evaluateAutoRotation(&imu, &manager, &matrix);

    // Regression guard: toggling auto-rotate back on must clear the cached
    // last orientation so the current pose is applied again immediately.
    TEST_ASSERT_EQUAL_UINT32(2, matrix.setRotationCalls);
    TEST_ASSERT_EQUAL_UINT8(1, matrix.lastRotation);
    TEST_ASSERT_EQUAL_UINT32(3, g_setConsumerCalls);
    TEST_ASSERT_TRUE(g_lastConsumerActive);
}

void test_effect_input_activates_imu_consumer_for_native_3d_provider() {
    MatrixService matrix;
    ImuService imu;
    IMU::ImuManager manager(&imu);

    RTC::mockStore.matrix.effectEnabled = true;
    RTC::mockStore.matrix.effectEngine = 1;
    RTC::mockStore.matrix.effectReactivityProvider = 1;
    g_hasSample = true;
    g_sample.ax = 0.25f;
    g_sample.ay = -0.50f;
    g_sample.az = 0.90f;
    g_sample.gx = 45.0f;
    g_sample.gy = -20.0f;
    g_sample.gz = 10.0f;
    g_sample.timestampMs = 700;
    g_sample.valid = true;
    TEST_STUBS::ARDUINO::millisValue = 750;

    MATRIX::MatrixTask::evaluateEffectInput(&imu, &manager, &matrix);

    TEST_ASSERT_EQUAL_UINT32(1, g_setConsumerCalls);
    TEST_ASSERT_EQUAL(IMU::Consumer::MatrixEffects, g_lastConsumer);
    TEST_ASSERT_TRUE(g_lastConsumerActive);
    TEST_ASSERT_EQUAL_UINT32(1, matrix.setEffectInputCalls);
    TEST_ASSERT_TRUE(matrix.lastEffectInput.imuValid);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, matrix.lastEffectInput.ax);
    TEST_ASSERT_EQUAL_FLOAT(-0.50f, matrix.lastEffectInput.ay);
    TEST_ASSERT_EQUAL_FLOAT(0.90f, matrix.lastEffectInput.az);
    TEST_ASSERT_EQUAL_FLOAT(45.0f, matrix.lastEffectInput.gx);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, matrix.lastEffectInput.motionEnergy);
    TEST_ASSERT_EQUAL_UINT32(750, matrix.lastEffectInput.timestampMs);
}

void test_effect_input_disables_imu_consumer_when_provider_no_longer_needs_it() {
    MatrixService matrix;
    ImuService imu;
    IMU::ImuManager manager(&imu);

    RTC::mockStore.matrix.effectEnabled = true;
    RTC::mockStore.matrix.effectEngine = 1;
    RTC::mockStore.matrix.effectReactivityProvider = 1;
    g_hasSample = true;
    g_sample.ax = 0.0f;
    g_sample.ay = 0.0f;
    g_sample.az = 1.0f;

    MATRIX::MatrixTask::evaluateEffectInput(&imu, &manager, &matrix);
    RTC::mockStore.matrix.effectEnabled = false;
    MATRIX::MatrixTask::evaluateEffectInput(&imu, &manager, &matrix);

    TEST_ASSERT_EQUAL_UINT32(2, g_setConsumerCalls);
    TEST_ASSERT_EQUAL(IMU::Consumer::MatrixEffects, g_lastConsumer);
    TEST_ASSERT_FALSE(g_lastConsumerActive);
    TEST_ASSERT_EQUAL_UINT32(2, matrix.setEffectInputCalls);
    TEST_ASSERT_FALSE(matrix.lastEffectInput.imuValid);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_auto_rotate_reapplies_rotation_after_toggle);
    RUN_TEST(test_effect_input_activates_imu_consumer_for_native_3d_provider);
    RUN_TEST(test_effect_input_disables_imu_consumer_when_provider_no_longer_needs_it);
    return UNITY_END();
}

#endif
