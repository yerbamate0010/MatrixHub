/**
 * @file PsramAllocator.h
 * @brief Standard Allocator for STL containers to use PSRAM
 */

#pragma once

#include <cstddef>
#include <limits>
#include <new>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>

namespace SYSTEM {

template <class T>
struct PsramAllocator {
    typedef T value_type;

    PsramAllocator() = default;
    
    template <class U>
    constexpr PsramAllocator(const PsramAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T)) {
            ESP_LOGE("PsramAlloc", "Alloc size overflow or too large: %u * %u", (unsigned)n, (unsigned)sizeof(T));
            throw std::bad_alloc();
        }
        
        void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!ptr) {
            ESP_LOGE("PsramAlloc", "MALLOC FAILED: %u bytes", (unsigned)(n * sizeof(T)));
            throw std::bad_alloc();
        }
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t) noexcept {
        free(p);
    }
};

template <class T, class U>
bool operator==(const PsramAllocator<T>&, const PsramAllocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const PsramAllocator<T>&, const PsramAllocator<U>&) { return false; }

using PsramString = std::basic_string<char, std::char_traits<char>, PsramAllocator<char>>;
template <typename T> using PsramVector = std::vector<T, PsramAllocator<T>>;

inline PsramString makePsramString(const char* text) {
    if (!text || !*text) return PsramString(PsramAllocator<char>());
    return PsramString(text, PsramAllocator<char>());
}
inline PsramString makePsramString(const std::string& text) {
    return PsramString(text.c_str(), PsramAllocator<char>());
}
inline PsramString makePsramString(const String& text) {
    return PsramString(text.c_str(), PsramAllocator<char>());
}

// --- ARDUINOJSON V7 PSRAM ALLOCATOR ---

class JsonPsramAllocator : public ArduinoJson::Allocator {
public:
    void* allocate(size_t size) override {
        // PSRAM-only allocation
        void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!ptr) {
             ESP_LOGE("PsramAlloc", "PSRAM allocation failed: %u bytes", (unsigned)size);
        }
        return ptr;
    }

    void deallocate(void* ptr) override {
        free(ptr);
    }

    void* reallocate(void* ptr, size_t new_size) override {
        if (new_size == 0) {
            free(ptr);
            return nullptr;
        }

        void* new_ptr = heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!new_ptr) {
            ESP_LOGE("PsramAlloc", "PSRAM reallocation failed: %u bytes", (unsigned)new_size);
        }
        return new_ptr;
    }

};

// Shared ArduinoJson allocator backed by PSRAM. This is a process-local helper
// rather than a semantic singleton service, so expose it as a free accessor.
inline JsonPsramAllocator& jsonPsramAllocator() {
    static JsonPsramAllocator allocator;
    return allocator;
}

// API compatibility wrapper (accepts size_t in constructor)
class SpiRamJsonDocument : public ArduinoJson::JsonDocument {
public:
    // In v7 size is dynamic, but we keep the capacity argument for compatibility
    // with code like `doc(8192)`.
    explicit SpiRamJsonDocument(size_t capacity = 0)
      : ArduinoJson::JsonDocument(&jsonPsramAllocator()) {
        // v7 manages memory automatically. Capacity hint is ignored.
        (void)capacity; 
    }

    SpiRamJsonDocument(const SpiRamJsonDocument& src) : ArduinoJson::JsonDocument(src) {}
    SpiRamJsonDocument(SpiRamJsonDocument&& src) : ArduinoJson::JsonDocument(std::move(src)) {}
};

} // namespace SYSTEM
