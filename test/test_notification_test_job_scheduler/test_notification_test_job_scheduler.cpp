#ifdef UNIT_TEST

#include <unity.h>

#include "../../test/stubs/PsychicHttpServer.h"
#include "../../test/stubs/PsychicJson.h"
#include "../../test/stubs/PsychicRequest.h"
#include "../../test/stubs/freertos/task.h"
#include "../../test/stubs/freertos/semphr.h"

#define SVK_TAG "Test"

#define private public
#include "../../src/api/common/TestRequestRateLimiter.h"
#include "../../src/api/notifications/NotificationTestJobScheduler.h"
#undef private

#include "../../src/config/System.h"
#include "../../src/system/logging/Logging.h"
#include "../../src/system/rtc/RtcConfig.h"
#include "../../src/system/watchdog/TaskWatchdog.h"

namespace LOG {

Settings Logging::_settings{};

void Logging::log(esp_log_level_t level, const char* tag, const char* fmt, ...) {
    (void)level;
    (void)tag;
    (void)fmt;
}

}  // namespace LOG

namespace RTC {

RtcRuntimeStats runtimeStats{};

}  // namespace RTC

namespace SYSTEM {

void TaskWatchdog::begin(uint32_t timeoutSec, bool panicOnTimeout) {
    (void)panicOnTimeout;
    _timeoutSec = timeoutSec;
    _initialized = true;
}

bool TaskWatchdog::registerCurrentTask() {
    return true;
}

bool TaskWatchdog::registerTask(TaskHandle_t taskHandle) {
    (void)taskHandle;
    return true;
}

bool TaskWatchdog::unregisterCurrentTask() {
    return true;
}

bool TaskWatchdog::unregisterTask(TaskHandle_t taskHandle) {
    (void)taskHandle;
    return true;
}

bool TaskWatchdog::reset() {
    return true;
}

}  // namespace SYSTEM

#include "../../src/api/notifications/NotificationTestJobScheduler.cpp"

namespace API {

TelegramTestSender::TelegramTestSender(NotificationSettingsService* settingsService,
                                       TELEGRAM::TelegramClient* client,
                                       TELEGRAM::TelegramWorker* worker)
    : _settingsService(settingsService)
    , _client(client)
    , _worker(worker) {}

SendTestResult TelegramTestSender::sendTest(const char* text, size_t textLen) {
    (void)text;
    (void)textLen;
    return {};
}

bool TelegramTestSender::isConfigured() const {
    return true;
}

WebhookTestSender::WebhookTestSender(NotificationSettingsService* settingsService,
                                     NOTIFICATIONS::WebhookTransportService* transport)
    : _settingsService(settingsService)
    , _transport(transport) {}

WebhookSendTestResult WebhookTestSender::sendTest(const char* payload, size_t payloadLen) {
    (void)payload;
    (void)payloadLen;
    return {};
}

bool WebhookTestSender::isConfigured() const {
    return true;
}

PushoverTestSender::PushoverTestSender(NotificationSettingsService* settingsService,
                                       NOTIFICATIONS::PushoverTransportService* transport)
    : _settingsService(settingsService)
    , _transport(transport) {}

PushoverSendTestResult PushoverTestSender::sendTest(const char* message, size_t messageLen) {
    (void)message;
    (void)messageLen;
    return {};
}

bool PushoverTestSender::isConfigured() const {
    return true;
}

}  // namespace API

namespace {

API::TelegramTestSender* telegramSender() {
    return reinterpret_cast<API::TelegramTestSender*>(0x1);
}

API::WebhookTestSender* webhookSender() {
    return reinterpret_cast<API::WebhookTestSender*>(0x1);
}

API::PushoverTestSender* pushoverSender() {
    return reinterpret_cast<API::PushoverTestSender*>(0x1);
}

void resetRateLimiter() {
    API::TESTS::testRequestRateLimiter()._busyUntilMs.store(0, std::memory_order_relaxed);
}

void releaseSchedulerSemaphores(API::NotificationTestJobScheduler& scheduler) {
    if (scheduler._telegramSemaphore) {
        xSemaphoreGive(scheduler._telegramSemaphore);
    }
    if (scheduler._webhookSemaphore) {
        xSemaphoreGive(scheduler._webhookSemaphore);
    }
    if (scheduler._pushoverSemaphore) {
        xSemaphoreGive(scheduler._pushoverSemaphore);
    }
}

}  // namespace

void setUp(void) {
    TEST_STUBS::FREERTOS::resetTaskCreateStub();
    TEST_STUBS::FREERTOS::resetSemaphoreStubs();
    resetRateLimiter();
    RTC::runtimeStats = {};
}

void tearDown(void) {}

void test_begin_creates_all_channel_semaphores() {
    API::NotificationTestJobScheduler scheduler;

    TEST_ASSERT_TRUE(scheduler.begin());
    TEST_ASSERT_NOT_NULL(scheduler._telegramSemaphore);
    TEST_ASSERT_NOT_NULL(scheduler._webhookSemaphore);
    TEST_ASSERT_NOT_NULL(scheduler._pushoverSemaphore);
}

void test_schedule_telegram_queues_task_with_expected_runtime_params() {
    API::NotificationTestJobScheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.begin());

    const auto result = scheduler.scheduleTelegram("ping", telegramSender());

    TEST_ASSERT_TRUE(result.isQueued());
    TEST_ASSERT_EQUAL_STRING("tg_test", TEST_STUBS::FREERTOS::lastTaskName);
    TEST_ASSERT_EQUAL_UINT32(CONFIG::TASKS::STACK_NOTIFICATION_TEST,
                             TEST_STUBS::FREERTOS::lastTaskStackDepth);
    TEST_ASSERT_EQUAL_UINT32(CONFIG::TASKS::PRIO_NOTIFICATION_TEST,
                             TEST_STUBS::FREERTOS::lastTaskPriority);
    TEST_ASSERT_EQUAL_INT(CONFIG::TASKS::CORE_NOTIFICATION_TEST,
                          TEST_STUBS::FREERTOS::lastTaskCore);
    releaseSchedulerSemaphores(scheduler);
}

void test_schedule_telegram_returns_busy_when_channel_semaphore_is_taken() {
    API::NotificationTestJobScheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.begin());
    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreTake(scheduler._telegramSemaphore, 0));

    const auto result = scheduler.scheduleTelegram("ping", telegramSender());

    TEST_ASSERT_EQUAL_INT(static_cast<int>(API::NotificationTestScheduleStatus::Busy),
                          static_cast<int>(result.status));
    releaseSchedulerSemaphores(scheduler);
}

void test_schedule_telegram_returns_rate_limited_when_global_cooldown_is_active() {
    API::NotificationTestJobScheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.begin());
    TEST_ASSERT_TRUE(API::TESTS::testRequestRateLimiter().tryAcquire());

    const auto result = scheduler.scheduleTelegram("ping", telegramSender());

    TEST_ASSERT_EQUAL_INT(static_cast<int>(API::NotificationTestScheduleStatus::RateLimited),
                          static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(API::TESTS::GLOBAL_COOLDOWN_MS, result.retryAfterMs);
}

void test_schedule_telegram_releases_semaphore_after_task_create_failure() {
    API::NotificationTestJobScheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.begin());

    TEST_STUBS::FREERTOS::taskCreateResult = pdFAIL;
    const auto failed = scheduler.scheduleTelegram("ping", telegramSender());

    TEST_ASSERT_EQUAL_INT(static_cast<int>(API::NotificationTestScheduleStatus::TaskCreateFailed),
                          static_cast<int>(failed.status));

    TEST_STUBS::FREERTOS::taskCreateResult = pdPASS;
    resetRateLimiter();
    const auto queued = scheduler.scheduleTelegram("retry", telegramSender());

    TEST_ASSERT_TRUE(queued.isQueued());
    releaseSchedulerSemaphores(scheduler);
}

void test_schedule_webhook_and_pushover_use_channel_specific_task_names() {
    API::NotificationTestJobScheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.begin());

    const auto webhook = scheduler.scheduleWebhook("{\"ok\":true}", webhookSender());

    TEST_ASSERT_TRUE(webhook.isQueued());
    TEST_ASSERT_EQUAL_STRING("wh_test", TEST_STUBS::FREERTOS::lastTaskName);
    TEST_ASSERT_EQUAL_UINT32(CONFIG::TASKS::STACK_NOTIFICATION_TEST,
                             TEST_STUBS::FREERTOS::lastTaskStackDepth);

    TEST_ASSERT_EQUAL(pdTRUE, xSemaphoreGive(scheduler._webhookSemaphore));
    resetRateLimiter();

    const auto pushover = scheduler.schedulePushover("hello", pushoverSender());

    TEST_ASSERT_TRUE(pushover.isQueued());
    TEST_ASSERT_EQUAL_STRING("po_test", TEST_STUBS::FREERTOS::lastTaskName);
    TEST_ASSERT_EQUAL_UINT32(CONFIG::TASKS::STACK_NOTIFICATION_TEST,
                             TEST_STUBS::FREERTOS::lastTaskStackDepth);
    releaseSchedulerSemaphores(scheduler);
}

void test_schedule_methods_return_service_unavailable_without_dependencies() {
    API::NotificationTestJobScheduler scheduler;
    TEST_ASSERT_TRUE(scheduler.begin());

    const auto telegram = scheduler.scheduleTelegram("ping", nullptr);
    const auto webhook = scheduler.scheduleWebhook("{}", nullptr);
    const auto pushover = scheduler.schedulePushover("hello", nullptr);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(API::NotificationTestScheduleStatus::ServiceUnavailable),
                          static_cast<int>(telegram.status));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(API::NotificationTestScheduleStatus::ServiceUnavailable),
                          static_cast<int>(webhook.status));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(API::NotificationTestScheduleStatus::ServiceUnavailable),
                          static_cast<int>(pushover.status));
}

void test_destructor_releases_idle_channel_semaphores() {
    {
        API::NotificationTestJobScheduler scheduler;
        TEST_ASSERT_TRUE(scheduler.begin());
        TEST_ASSERT_NOT_NULL(scheduler._telegramSemaphore);
        TEST_ASSERT_NOT_NULL(scheduler._webhookSemaphore);
        TEST_ASSERT_NOT_NULL(scheduler._pushoverSemaphore);
    }

    TEST_ASSERT_TRUE(TEST_STUBS::FREERTOS::binarySemaphores().empty());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_begin_creates_all_channel_semaphores);
    RUN_TEST(test_schedule_telegram_queues_task_with_expected_runtime_params);
    RUN_TEST(test_schedule_telegram_returns_busy_when_channel_semaphore_is_taken);
    RUN_TEST(test_schedule_telegram_returns_rate_limited_when_global_cooldown_is_active);
    RUN_TEST(test_schedule_telegram_releases_semaphore_after_task_create_failure);
    RUN_TEST(test_schedule_webhook_and_pushover_use_channel_specific_task_names);
    RUN_TEST(test_schedule_methods_return_service_unavailable_without_dependencies);
    RUN_TEST(test_destructor_releases_idle_channel_semaphores);
    return UNITY_END();
}

#endif  // UNIT_TEST
