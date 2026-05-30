#include "HeartbeatApiService.h"

#include "../../system/health/heartbeat/Heartbeat.h"
#include "../../system/utils/json/JsonResponseWriter.h"
#include "../common/TestRequestRateLimiter.h"
#include <PsychicJson.h>
#include <utils/ResponseUtils.h>

namespace API {

HeartbeatApiService::HeartbeatApiService(PsychicHttpServer* server,
                                         SecurityManager* securityManager,
                                         POWER::PowerManager* powerManager,
                                         SYSTEM::HeartbeatSettingsService* settings)
    : BaseApiService(server, securityManager, powerManager, "api/heartbeat"),
      _settings(settings) {
    // Heartbeat settings now come from ServiceRegistry. The API stays focused
    // on transport while the settings service retains ownership of persistence
    // and post-save apply logic.
    if (_settings) {
        _configEndpoint = std::make_unique<HttpEndpoint<RTC::HeartbeatData>>(
            SYSTEM::HeartbeatSettingsService::readState,
            SYSTEM::HeartbeatSettingsService::updateState,
            _settings,
            _server,
            "/api/heartbeat",
            _securityManager,
            AuthenticationPredicates::IS_ADMIN,
            nullptr,
            [this]() {
                if (_powerManager) {
                    _powerManager->notifyActivity(_activityTag);
                }
            });
    }
}

void HeartbeatApiService::begin() {
    if (_configEndpoint) {
        _configEndpoint->begin();
    }
    
    // POST /api/heartbeat/test
    _server->on("/api/heartbeat/test", HTTP_POST,
        wrapAdmin([this](PsychicRequest* request) -> esp_err_t {
            RTC::HeartbeatData cfg = SYSTEM::HEARTBEAT_CONFIG::copy();
            bool anyEnabled = false;
            for (uint8_t i = 0; i < RTC::kMaxHeartbeatSlots; i++) {
                if (cfg.slots[i].enabled && cfg.slots[i].url[0] != '\0') {
                    anyEnabled = true;
                    break;
                }
            }
            
            if (!anyEnabled) {
                return Response::error(request, 400, "heartbeat/no_enabled_slots");
            }

            if (!TESTS::testRequestRateLimiter().tryAcquire()) {
                const uint32_t retryAfterMs = TESTS::testRequestRateLimiter().remainingMs();
                return Response::error(request, 429, TESTS::kBusyErrorCode, [retryAfterMs](JsonVariant& root) {
                    root["success"] = false;
                    root["message"] = TESTS::kBusyErrorCode;
                    root["retry_after_ms"] = retryAfterMs;
                });
            }
            
            if (SYSTEM::Heartbeat::pingNow()) {
                Utils::JsonResponseWriter w(request->request());
                if (!w.beginResponse()) return ESP_FAIL;
                w.raw("{");
                w.key("success"); w.value(true);
                w.raw(","); w.key("message"); w.string("ping_triggered");
                w.raw("}");
                w.finish();
                return ESP_OK;
            } else {
                return Response::error(request, 400, "heartbeat/ping_failed");
            }
        })
    );
}

}  // namespace API
