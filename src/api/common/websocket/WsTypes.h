#pragma once

#include <cstdint>
#include <esp_http_server.h>

namespace API {
namespace WEBSOCKET {

constexpr size_t MAX_BROADCAST_TARGETS = 32;

struct WsMessage {
    uint8_t* data;
    size_t len;
    httpd_ws_type_t type;
    bool isAllocated;
    int16_t payloadSlot;
    int targets[MAX_BROADCAST_TARGETS]; // Specific FDs to target (0 = all)
    size_t targetCount;
};

} // namespace WEBSOCKET
} // namespace API
