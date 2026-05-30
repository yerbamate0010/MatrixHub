#include "MatrixLayerManager.h"
#include "../utils/ScopeLock.h"

namespace MATRIX_MANAGER {

MatrixLayerManager::MatrixLayerManager() {
    _mutex = xSemaphoreCreateMutexStatic(&_mutexBuffer);
    for (uint8_t i = 0; i < LAYER_COUNT; i++) {
        _layers[i].clear();
    }
}

void MatrixLayerManager::setLayer(Layer layer, const LayerContent& content) {
    SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(50));
    if (!lock.isLocked()) return;

    uint8_t idx = static_cast<uint8_t>(layer);
    if (idx >= LAYER_COUNT) return;

    _layers[idx] = content;
    _layers[idx].active = true;
}

void MatrixLayerManager::clearLayer(Layer layer) {
    SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(50));
    if (!lock.isLocked()) return;

    uint8_t idx = static_cast<uint8_t>(layer);
    if (idx >= LAYER_COUNT) return;

    _layers[idx].clear();
}

bool MatrixLayerManager::getTopLayer(LayerContent& out, Layer& outLayer) const {
    SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(50));
    if (!lock.isLocked()) return false;

    // Iterate from highest priority (ALARM=5) down to BACKGROUND=0
    for (int8_t i = LAYER_COUNT - 1; i >= 0; i--) {
        if (_layers[i].active) {
            out = _layers[i];
            outLayer = static_cast<Layer>(i);
            return true;
        }
    }

    return false;
}

bool MatrixLayerManager::isLayerActive(Layer layer) const {
    SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(50));
    if (!lock.isLocked()) return false;

    uint8_t idx = static_cast<uint8_t>(layer);
    if (idx >= LAYER_COUNT) return false;

    return _layers[idx].active;
}

bool MatrixLayerManager::getLayerContent(Layer layer, LayerContent& out) const {
    SYSTEM::ScopeLock lock(_mutex, pdMS_TO_TICKS(50));
    if (!lock.isLocked()) return false;

    uint8_t idx = static_cast<uint8_t>(layer);
    if (idx >= LAYER_COUNT) return false;

    if (_layers[idx].active) {
        out = _layers[idx];
        return true;
    }
    return false;
}

} // namespace MATRIX_MANAGER
