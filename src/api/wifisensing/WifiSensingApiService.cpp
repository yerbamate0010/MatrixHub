#include "WifiSensingApiService.h"

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
             if (count == 0) return;

             // The worker may batch packets for efficiency, but the WebSocket wire
             // protocol still emits one CSI frame per message. That keeps browser-side
             // parsing and chart cadence stable even if batching changes internally.
             for (size_t i = 0; i < count; i++) {
                 const auto& packet = batch[i];
                 _csiEndpoint.broadcaster().broadcastSerialized(
                     CSI_BROADCAST_BUFFER_SIZE,
                     [&packet](uint8_t* buffer, size_t capacity) -> size_t {
                         size_t offset = 0;
                         uint16_t dataLen = packet.len;
                         if (dataLen > WIFISENSING::CSI::MAX_CSI_DATA_LEN) {
                             dataLen = WIFISENSING::CSI::MAX_CSI_DATA_LEN;
                         }

                         if (capacity < CSI_FRAME_HEADER_BYTES + dataLen) {
                             return 0;
                         }

                         // Keep this byte layout aligned with
                         // docs/engineering/integrations/csi.md and the
                         // frontend parser in interface/src/lib/features/wifisensing/csi/.
                         uint32_t ts = packet.rx_ctrl.timestamp;
                         memcpy(&buffer[offset], &ts, 4); offset += 4;
                         buffer[offset++] = static_cast<uint8_t>(packet.rx_ctrl.rssi);
                         memcpy(&buffer[offset], &dataLen, 2); offset += 2;

                         const float gainTimesTen = packet.compensate_gain * 10.0f;
                         const uint8_t encodedGain =
                             (gainTimesTen <= 0.0f) ? 0 : (gainTimesTen >= 255.0f ? 255 : static_cast<uint8_t>(gainTimesTen));
                         buffer[offset++] = encodedGain;

                         memcpy(&buffer[offset], &packet.motionScore, 4); offset += 4;
                         buffer[offset++] = packet.isMotionDetected ? 1 : 0;

                         memcpy(&buffer[offset], packet.buf, dataLen);
                         offset += dataLen;
                         return offset;
                     });
             }
        });
    }

}

bool WifiSensingApiService::ensureCsiTransportReady() {
    // The queue is enabled lazily on first client so CSI does not keep its
    // transport machinery alive when no frontend is listening.
    if (!_csiEndpoint.broadcaster().isQueueEnabled()) {
        _csiEndpoint.broadcaster().enableQueue(CSI_WS_QUEUE_DEPTH, 4096, CSI_BROADCAST_BUFFER_SIZE);
        if (!_csiEndpoint.broadcaster().isQueueEnabled()) {
            LOGE("Failed to enable CSI WebSocket queue");
            return false;
        }
    }
    LOGI("CSI transport ready: frame=%u bytes, queue depth=%u", CSI_BROADCAST_BUFFER_SIZE, CSI_WS_QUEUE_DEPTH);
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
