#include "MatrixApiService.h"

#include <PsychicJson.h>
#include <utils/ResponseUtils.h>

#include "../../system/logging/Logging.h"

#undef LOG_TAG
#define LOG_TAG "MatrixApi"

namespace API {

namespace {
constexpr const char* kMatrixSettingsPath = "/api/matrix/settings";
}

MatrixApiService::MatrixApiService(
    PsychicHttpServer* server,
    SecurityManager* securityManager,
    POWER::PowerManager* powerManager,
    MATRIX::MatrixSettingsService* matrixSettings)
    : BaseApiService(server, securityManager, powerManager, "api/matrix"),
      _matrixSettings(matrixSettings) {
    if (_matrixSettings) {
        _configEndpoint = std::make_unique<HttpEndpoint<MATRIX::MatrixSettingsState>>(
            MATRIX::MatrixSettingsService::readState,
            MATRIX::MatrixSettingsService::updateState,
            _matrixSettings,
            _server,
            kMatrixSettingsPath,
            _securityManager,
            AuthenticationPredicates::IS_ADMIN,
            AuthenticationPredicates::IS_AUTHENTICATED,
            nullptr,
            [this]() {
                if (_powerManager) {
                    _powerManager->notifyActivity(_activityTag);
                }
            });
    }
}

void MatrixApiService::begin() {
    if (!_configEndpoint || !_matrixSettings) {
        LOGE("Matrix settings endpoint was not initialized");
        return;
    }

    _configEndpoint->begin();
}

}  // namespace API
