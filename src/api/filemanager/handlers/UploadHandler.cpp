#include "api/filemanager/handlers/UploadHandler.h"
#include "config/App.h"
#include "config/System.h"
#include "system/errors/ErrorCodes.h"
#include "system/utils/json/JsonResponseWriter.h"
#include "system/utils/http/HttpError.h"
#include "filemanager/infrastructure/support/FileManagerPathUtils.h"
#include "filemanager/infrastructure/uploads/UploadSession.h"

#include "system/logging/Logging.h"
#undef LOG_TAG
#define LOG_TAG "FmUpload"

namespace API {

UploadHandler::UploadHandler(FILEMGR::FileManagerBackendResolver* resolver, 
                             FILEMGR::StorageService* storage, 
                             SemaphoreHandle_t fsMutex)
  : _backendResolver(resolver), _storage(storage), _fsMutex(fsMutex) {
  _sessionsMutex = xSemaphoreCreateMutex();
}

UploadHandler::~UploadHandler() {
  if (_sessionsMutex) {
    xSemaphoreTake(_sessionsMutex, portMAX_DELAY);
    _uploadSessions.clear(); // Automatically calls PsramDeleter for all sessions
    xSemaphoreGive(_sessionsMutex);
    vSemaphoreDelete(_sessionsMutex);
  }
}

void UploadHandler::cleanupStaleSessions() {
    uint32_t now = millis();
    for (auto it = _uploadSessions.begin(); it != _uploadSessions.end(); ) {
        if (now - it->second->getLastActivity() > CONFIG::FILEMGR::kUploadTimeoutMs) {
            // Fallback cleanup path. Normal disconnects are handled via
            // handleClientClose(), but stale-session sweeping still protects us
            // from interrupted uploads that never receive a close callback.
            LOGW("Cleaning up stale upload session for client %d", it->first);
            it = _uploadSessions.erase(it); // Automatically releases PSRAM and cleans up .tmp file
        } else {
            ++it;
        }
    }
}

esp_err_t UploadHandler::handleChunk(PsychicRequest* request, const char* filename,
                                     uint64_t index, uint8_t* data, size_t len, bool final) {
  int clientId = request->client()->socket();

  // Intentional tradeoff: this device exposes a single-user embedded admin panel,
  // not a multi-tenant upload service. We keep full chunk handling serialized under
  // _sessionsMutex to preserve simple upload-session ownership and cleanup semantics,
  // and accept reduced concurrent upload throughput as a product-level compromise.
  // Revisit only if parallel uploads become a real requirement.
  xSemaphoreTake(_sessionsMutex, portMAX_DELAY);
  cleanupStaleSessions(); // Periodically clean up stale sessions

  if (index == 0) {
    _uploadSessions.erase(clientId); // Clean up any existing session for this client

    PsychicWebParameter* pathParam = request->getParam(CONFIG::Keys::kFmParamPath);
    static const String kRootPath("/");
    const String& requestedPath = pathParam ? pathParam->value() : kRootPath;
    SYSTEM::PsramString canonicalRequestedPath;
    if (!FILEMGR::FileManagerPathUtils::canonicalizeAbsoluteDirectory(
            requestedPath.c_str(), "/", canonicalRequestedPath)) {
      LOGW("Upload rejected: invalid path=%s", requestedPath.c_str());
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 404, ErrorCodes::Fs::INVALID_PATH);
    }

    FILEMGR::FileManagerBackendResolution resolution = _backendResolver->resolveForUpload(canonicalRequestedPath);
    if (!resolution.filesystem) {
      LOGW("Upload failed: no filesystem resolved for path=%s", canonicalRequestedPath.c_str());
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 404, ErrorCodes::Fs::INVALID_PATH);
    }

    if (!FILEMGR::FileManagerPathUtils::isAccessAllowed(
            resolution.nativePath.c_str(), FILEMGR::FileManagerPathUtils::FileManagerPathAccess::Upload)) {
      LOGW("Upload rejected by path policy: request=%s native=%s",
           canonicalRequestedPath.c_str(), resolution.nativePath.c_str());
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 403, ErrorCodes::Fs::PATH_FORBIDDEN);
    }

    FILEMGR::StorageMetrics metrics = _storage->getMetricsForPath(canonicalRequestedPath);
    size_t freeSpace = metrics.freeBytes();
    
    if (freeSpace < CONFIG::Keys::FILEMGR::kUploadMinFreeSpace) {
      LOGW("Upload rejected: Insufficient flash space (%zu bytes)", freeSpace);
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 507, ErrorCodes::Fs::STORAGE_FULL);
    }

    // Reserve minimum free space after upload completes
    size_t usableSpace = freeSpace - CONFIG::Keys::FILEMGR::kUploadMinFreeSpace;

    // 1. Early detection of insufficient space based on Content-Length (HTTP 507)
    size_t requestSize = request->contentLength();
    if (requestSize > usableSpace) {
      LOGW("Upload rejected: Request size (%zu) exceeds usable space (%zu)", requestSize, usableSpace);
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 507, ErrorCodes::Fs::STORAGE_FULL);
    }

    size_t uploadLimit = std::min((size_t)CONFIG::Keys::FILEMGR::kUploadMaxFileSize, usableSpace);

    LOGI("Upload path=%s native=%s file=%s limit=%zu free=%zu usable=%zu client=%d",
         canonicalRequestedPath.c_str(),
         resolution.nativePath.c_str(),
         filename ? filename : "",
         uploadLimit,
         freeSpace,
         usableSpace,
         clientId);

    void* mem = heap_caps_malloc(sizeof(FILEMGR::UploadSession), MALLOC_CAP_SPIRAM);
    if (!mem) {
      LOGE("Upload failed: OOM in PSRAM");
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 500, ErrorCodes::Internal::OUT_OF_MEMORY);
    }
    
    _uploadSessions[clientId] = UploadSessionPtr(
        new (mem) FILEMGR::UploadSession(resolution.filesystem,
                                         resolution.nativePath.c_str(),
                                         _fsMutex,
                                         uploadLimit)
    );
  }

  auto it = _uploadSessions.find(clientId);
  if (it == _uploadSessions.end()) {
    xSemaphoreGive(_sessionsMutex);
    LOGW("Upload failed: no active session for client %d", clientId);
    return ESP_FAIL; 
  }

  esp_err_t err = it->second->handleChunk(request, filename, index, data, len, final);

  if (err == ESP_ERR_INVALID_STATE) {
      LOGW("Upload rejected: file already exists for client %d", clientId);
      _uploadSessions.erase(clientId);
      xSemaphoreGive(_sessionsMutex);
      return HttpError::send(request, 409, ErrorCodes::Fs::ALREADY_EXISTS);
  }

  if (err != ESP_OK) {
      LOGE("Error writing chunk, aborting session for client %d", clientId);
      _uploadSessions.erase(clientId); // Triggers destructor cleanup
  }

  xSemaphoreGive(_sessionsMutex);
  return err;
}

esp_err_t UploadHandler::handleComplete(PsychicRequest* request) {
  int clientId = request->client()->socket();
  bool ok = false;

  xSemaphoreTake(_sessionsMutex, portMAX_DELAY);
  auto it = _uploadSessions.find(clientId);
  if (it != _uploadSessions.end()) {
    ok = it->second->isSuccess(); // Check if file finalized successfully
    _uploadSessions.erase(it);    // Releases session memory
  }
  xSemaphoreGive(_sessionsMutex);

  if (ok) {
    LOGI("Upload fully successful for client %d", clientId);
    Utils::JsonResponseWriter w(request->request());
    if (!w.beginResponse()) return ESP_ERR_HTTPD_RESP_SEND;
    w.raw("{");
    w.key(CONFIG::Keys::kFmOk); w.value(true);
    w.raw("}");
    return w.finish() ? ESP_OK : ESP_FAIL;
  } else {
    LOGW("Upload completion failed or aborted for client %d", clientId);
    return HttpError::send(request, 500, ErrorCodes::Fs::UPLOAD_FAILED);
  }
}

void UploadHandler::handleClientClose(int clientId) {
  xSemaphoreTake(_sessionsMutex, portMAX_DELAY);

  auto it = _uploadSessions.find(clientId);
  if (it != _uploadSessions.end()) {
    // Disconnect is the earliest reliable signal that an interrupted upload
    // will never reach handleComplete(). Dropping the session here lets the
    // UploadSession destructor remove the .tmp file immediately instead of
    // waiting for the next upload or process teardown. This is production
    // hardening, not new behavior: the upload was already dead, we now just
    // reclaim its resources sooner and more deterministically.
    LOGW("Upload client disconnected, cleaning up session for client %d", clientId);
    _uploadSessions.erase(it);
  }

  xSemaphoreGive(_sessionsMutex);
}

}  // namespace API
