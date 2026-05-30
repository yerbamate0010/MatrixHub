/**
 * @file test_macro_api_service.cpp
 * @brief Integration-like tests for MacroApiService settings validation
 */

#include <unity.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <string>

#include "../../src/system/logging/Logging.h"
#include "../../src/system/rtc/RtcConfig.h"
#include "../../test/stubs/PsychicRequest.h"
#include "../../test/stubs/PsychicHttpServer.h"
#include "../../test/stubs/freertos/semphr.h"

// ----------------- Stubs -----------------
// Minimal HTTP server stubs
extern "C" {
    #include "esp_http_server.h"
    esp_err_t httpd_resp_set_type(httpd_req_t *r, const char *type) { return ESP_OK; }
    esp_err_t httpd_resp_set_hdr(httpd_req_t *r, const char *field, const char *value) { return ESP_OK; }
    esp_err_t httpd_resp_send_chunk(httpd_req_t *r, const char *buf, ssize_t len) { return ESP_OK; }
}

// Logging stub
namespace LOG {
    Settings Logging::_settings;
    void Logging::log(esp_log_level_t level, const char *tag, const char *fmt, ...) {}
    void Logging::logSection(const char* title) {}
    void Logging::logStackHwm(const char* taskName, uint32_t stackSize) {}
}

// RTC mock
namespace RTC {
    ConfigStore mockStore;

    ConfigStore& getMutableConfig() { return mockStore; }
    const ConfigStore& getConfig() { return mockStore; }
    void withConfig(const std::function<void(const ConfigStore&)>& reader) { reader(mockStore); }
    SemaphoreHandle_t getLock() {
        static SemaphoreHandle_t lock = xSemaphoreCreateMutex();
        return lock;
    }
    bool updateConfig(const std::function<void(ConfigStore&)>& updater) {
        updater(mockStore);
        return true;
    }
    void markValid() {}
}

// CONFIG mock
namespace CONFIG {
    bool save(FS& fs) { return true; }
}

// LittleFS global
FS LittleFS;

// Bring real implementations needed by MacroApiService
#include "../../src/system/utils/json/JsonResponseWriter.cpp"
#include "../../src/config/json/MacroConfigJson.cpp"
#include "../../src/macros/MacroSettingsService.cpp"

#include "../../src/api/macros/MacroApiService.cpp"

update_handler_id_t StateUpdateHandlerInfo::currentUpdatedHandlerId = 0;
hook_handler_id_t StateHookHandlerInfo::currentHookHandlerId = 0;

// MacroService stubs to satisfy linker in native tests
namespace MACROS {
    PsramVector<PsramString> MacroService::listScripts() { return {}; }
    bool MacroService::deleteScript(const char* filename) { (void)filename; return false; }
    bool MacroService::saveScript(const char* filename, const char* content) { (void)filename; (void)content; return false; }
    PsramString MacroService::getScriptContent(const char* filename) { (void)filename; return PsramString(PsramAllocator<char>()); }
    bool MacroService::startScript(const char* filename) { (void)filename; return false; }
    void MacroService::stopScript() {}
    MacroState MacroService::getStatus() { return MacroState(); }
    void MacroService::applySettings() {}
}

// PowerManager stub (BaseApiService wrapper dependency)
namespace POWER {
    void PowerManager::notifyActivity(const char *source) { (void)source; }
}

using namespace MACROS;

void setUp(void) {
    auto& cfg = RTC::getMutableConfig().macros;
    cfg.enabled = true;
    cfg.bootDelay = 5000;
    memset(cfg.bootScript, 0, sizeof(cfg.bootScript));
}

void tearDown(void) {}

void test_save_settings_boot_script_missing() {
    PsychicHttpServer server;
    SecurityManager securityManager;
    MacroSettingsService settings(&LittleFS, nullptr);
    MacroApiService api(&server, &securityManager, nullptr, nullptr, &settings);
    api.begin();

    PsychicRequest req;
    JsonDocument doc;
    doc["boot_script"] = "missing.txt";
    std::string body;
    serializeJson(doc, body);
    req.setBody(body);

    esp_err_t err = server.invoke("/api/macros/settings", HTTP_POST, &req);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(400, req.lastStatusCode);
    TEST_ASSERT_TRUE(req.lastResponseBody.find("boot_script_not_found") != std::string::npos);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_save_settings_boot_script_missing);
    return UNITY_END();
}
