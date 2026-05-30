#pragma once
#include <stddef.h>
#include <stdint.h>

#define MALLOC_CAP_INTERNAL 0
#define MALLOC_CAP_8BIT 0
#define MALLOC_CAP_SPIRAM 0

#ifdef __cplusplus
extern "C" {
#endif
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
size_t heap_caps_get_total_size(uint32_t caps);
uint32_t esp_get_minimum_free_heap_size();
uint32_t esp_get_free_heap_size();
inline void* heap_caps_malloc(size_t size, uint32_t caps) { return malloc(size); }
inline void heap_caps_free(void* ptr) { free(ptr); }
#ifdef __cplusplus
}
#endif
