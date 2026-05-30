#pragma once

#include <memory>

#include <core/HttpEndpoint.h>

#include "../BaseApiService.h"
#include "../../matrix/MatrixSettingsService.h"

namespace API {

class MatrixApiService : public BaseApiService {
public:
    MatrixApiService(PsychicHttpServer* server, SecurityManager* securityManager, POWER::PowerManager* powerManager, MATRIX::MatrixSettingsService* matrixSettings);

    void begin() override;

private:
    MATRIX::MatrixSettingsService* _matrixSettings;
    std::unique_ptr<HttpEndpoint<MATRIX::MatrixSettingsState>> _configEndpoint;
};

} // namespace API
