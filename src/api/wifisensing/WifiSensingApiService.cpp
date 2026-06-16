#include "WifiSensingApiService.h"
#include "CsiWireFormat.h"

#include <esp_http_server.h>

#include "../../wifisensing/WifiSensingService.h"
#include "../../wifisensing/WifiSensingSettings.h"
#include "../../wifisensing/csi/core/CsiService.h"
#include "../../config/System.h"
#include "../../config/json/WifiSensingConfigJson.h"
#include "core/config/ConfigManager.h"
#include "../../system/rtc/RtcConfig.h"
#include "../../system/power/PowerManager.h"
#include "../../system/logging/Logging.h"
#include "../../system/utils/json/JsonResponseWriter.h"
#include <ArduinoJson.h>
#include <PsychicJson.h>
#include <utils/ResponseUtils.h>
#include <LittleFS.h>
#include <services/RestartService.h>
#include "../../system/errors/ErrorCodes.h"

#undef LOG_TAG
#define LOG_TAG "ApiWifiSensing"

namespace API {
namespace {
constexpr const char* kWifiSensingConfigPath = "/api/wifisensing/config";
constexpr size_t kCsiWsQueueDepth = 4;
constexpr uint32_t kCsiWsQueueStackBytes = 4096;
} // namespace

// Constructor
WifiSensingApiService::WifiSensingApiService(PsychicHttpServer* server, SecurityManager* securityManager, POWER::PowerManager* powerManager)
    : BaseApiService(server, securityManager, powerManager, "api/wifisensing"),
      _csiAuthenticator(securityManager),
      _csiEndpoint(server, CONFIG::Keys::kWsCsi, CONFIG::Keys::kCsiBroadcastName, &_csiAuthenticator, [this](bool start) {
          // CSI remains a dedicated data plane. We only unified lifecycle and
          // queue management; the wire format and the "frontend consumer active"
          // toggling are intentionally preserved.
          // Frontend is just one CSI consumer; future consumers (e.g. alarms)
          // can keep the service alive independently.
          if (start) {
              if (!ensureCsiTransportReady()) {
                  LOGE("Failed to prepare CSI WebSocket transport for frontend");
                  return;
              }
              if (_csiService) {
                  _csiService->setConsumerActive(WIFISENSING::CSI::CsiConsumer::Frontend, true);
                  LOGI("CSI frontend consumer: ACTIVE");
              }
              return;
          }

          if (_csiService) {
              _csiService->setConsumerActive(WIFISENSING::CSI::CsiConsumer::Frontend, false);
              LOGI("CSI frontend consumer: INACTIVE");
          }
          teardownCsiTransport();
      }, 50) { // <-- 50ms send timeout specifically for real-time CSI data streaming
    _csiEndpoint.setRequestCallback([this]() {
        if (_powerManager) {
            _powerManager->notifyActivity("ws/csi");
        }
    });
}

WifiSensingApiService::~WifiSensingApiService() {
    if (_csiService) {
        _csiService->setCsiCallback(nullptr);
    }
    teardownCsiTransport();
}

void WifiSensingApiService::injectComponents(WIFISENSING::WifiSensingSettings* settings,
                                  WIFISENSING::CSI::CsiService* csiService,
                                  WIFISENSING::WifiSensingService* wifiSensingService) {
    _settings = settings;
    _csiService = csiService;
    _wifiSensingService = wifiSensingService;

    if (_settings) {
        _configEndpoint = std::make_unique<HttpEndpoint<RTC::WifiSensingData>>(
            WIFISENSING::WifiSensingSettings::readState,
            WIFISENSING::WifiSensingSettings::updateState,
            _settings,
            _server,
            kWifiSensingConfigPath,
            _securityManager,
            AuthenticationPredicates::IS_ADMIN,
            AuthenticationPredicates::IS_AUTHENTICATED,
            nullptr,
            [this]() {
                if (_powerManager) {
                    _powerManager->notifyActivity(_activityTag);
                }
            });
    } else {
        _configEndpoint.reset();
    }
}

void WifiSensingApiService::begin() {
    LOGI("WiFi Sensing API endpoints registered");
    _csiEndpoint.begin();

    // --- REST Registration ---

    if (_configEndpoint) {
        _configEndpoint->begin();
    } else {
        LOGE("WiFi sensing config endpoint was not initialized");
    }

    // 4. Register CSI Callback
    if (_csiService) {
        _csiService->setCsiCallback([this](const WIFISENSING::CSI::CsiPacket* batch, size_t count) {
            if (!batch || count == 0) return;

            for (size_t offset = 0; offset < count;) {
                const size_t remaining = count - offset;
                const size_t batchCount =
                    (remaining > WIFISENSING::CSI::MAX_CSI_BATCH_PACKETS)
                        ? WIFISENSING::CSI::MAX_CSI_BATCH_PACKETS
                        : remaining;
                const auto* batchStart = batch + offset;
                const size_t reserveLen = API::CSI_WIRE::RECORD_MAX_BYTES * batchCount;

                // Keep this byte layout aligned with docs/engineering/integrations/csi.md
                // and interface/src/lib/features/wifisensing/csi/parseCsiFrame.ts.
                _csiEndpoint.broadcaster().broadcastSerialized(
                    reserveLen,
                    [batchStart, batchCount](uint8_t* buffer, size_t capacity) -> size_t {
                        return API::CSI_WIRE::writeBatch(buffer, capacity, batchStart, batchCount);
                    });

                offset += batchCount;
            }
        });
    }

}

bool WifiSensingApiService::ensureCsiTransportReady() {
    // The queue is enabled lazily on first client so CSI does not keep its
    // transport machinery alive when no frontend is listening.
    if (!_csiEndpoint.broadcaster().isQueueEnabled()) {
        _csiEndpoint.broadcaster().enableQueue(
            kCsiWsQueueDepth,
            kCsiWsQueueStackBytes,
            API::CSI_WIRE::BATCH_MAX_BYTES);
        if (!_csiEndpoint.broadcaster().isQueueEnabled()) {
            LOGE("Failed to enable CSI WebSocket queue");
            return false;
        }
    }
    LOGI("CSI transport ready: max payload=%u bytes, queue depth=%u",
         static_cast<unsigned>(API::CSI_WIRE::BATCH_MAX_BYTES),
         static_cast<unsigned>(kCsiWsQueueDepth));
    return true;
}

void WifiSensingApiService::teardownCsiTransport() {
    // When the last client leaves, we intentionally drop the dedicated queue.
    // That keeps CSI cheap when the feature is idle and mirrors the previous
    // first-client/last-client semantics.
    _csiEndpoint.broadcaster().disableQueue();
}

void WifiSensingApiService::cleanupClient(int fd) {
    _csiEndpoint.cleanupClient(fd);
}

}  // namespace API
