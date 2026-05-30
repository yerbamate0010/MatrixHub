#include "SystemSnapshots.h"

#include <ArduinoJson.h>

#include "../../../../system/logging/Logging.h"

#undef LOG_TAG
#define LOG_TAG "SysSnapTx"

namespace API::SYSTEM_WS {

bool sendSnapshotDoc(WebSocketBroadcaster* ws,
                     int fd,
                     SYSTEM::SpiRamJsonDocument& doc) {
    if (!ws) {
        return false;
    }

    // Fail closed if the JSON document overflowed while being assembled.
    // Sending a syntactically valid but truncated snapshot is worse than
    // dropping it here because the frontend would treat partial state as
    // authoritative and render inconsistent diagnostics/UI.
    if (doc.overflowed()) {
        const char* channel = doc["channel"].is<const char*>() ? doc["channel"].as<const char*>() : "unknown";
        LOGE("Snapshot JSON overflow for channel '%s'; dropping payload", channel);
        return false;
    }

    const size_t len = measureJson(doc);
    if (len == 0) {
        return false;
    }

    int target = fd;
    return ws->broadcastSerialized(
        &target,
        1,
        len,
        [&doc](uint8_t* buffer, size_t capacity) -> size_t {
            return serializeJson(doc, reinterpret_cast<char*>(buffer), capacity);
        },
        HTTPD_WS_TYPE_TEXT);
}

}  // namespace API::SYSTEM_WS
