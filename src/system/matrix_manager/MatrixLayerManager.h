#pragma once

#include "MatrixManagerTypes.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace MATRIX_MANAGER {

/**
 * @brief Manages Z-index layers for the Matrix display.
 * 
 * Each layer holds a LayerContent. Only the highest-priority active layer
 * is rendered at any time. Priority comes from the Layer enum value itself:
 * MENU is highest, BACKGROUND is lowest. Thread-safe via mutex.
 */
class MatrixLayerManager {
public:
    MatrixLayerManager();

    /// Set content on a layer (marks it active).
    void setLayer(Layer layer, const LayerContent& content);

    /// Clear a layer (marks it inactive).
    void clearLayer(Layer layer);

    /// Get the highest-priority active layer. Returns nullptr if all inactive.
    /// Caller must NOT hold this pointer across frames — copy the content.
    bool getTopLayer(LayerContent& out, Layer& outLayer) const;

    /// Check if a specific layer is active.
    bool isLayerActive(Layer layer) const;

    /// Get a copy of a specific layer's content, return true if active.
    bool getLayerContent(Layer layer, LayerContent& out) const;

private:
    LayerContent _layers[LAYER_COUNT];

    mutable SemaphoreHandle_t _mutex = nullptr;
    StaticSemaphore_t _mutexBuffer;
};

} // namespace MATRIX_MANAGER
