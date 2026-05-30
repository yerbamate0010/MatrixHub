#include "SystemApiSnapshots.h"

#include <WiFi.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <utility>
#include <wifi/WiFiSettingsService.h>

#include "../../../config/App.h"

#ifndef APP_VERSION
#define APP_VERSION APP::VERSION
#endif
#ifndef APP_NAME
#define APP_NAME APP::NAME
#endif
#ifndef BUILD_TARGET
#define BUILD_TARGET "esp32s3"
#endif

namespace API {

SystemTasksSnapshot::~SystemTasksSnapshot() {
    if (tasks) {
        heap_caps_free(tasks);
        tasks = nullptr;
    }
}

SystemTasksSnapshot::SystemTasksSnapshot(SystemTasksSnapshot&& other) noexcept {
    *this = std::move(other);
}

SystemTasksSnapshot& SystemTasksSnapshot::operator=(SystemTasksSnapshot&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (tasks) {
        heap_caps_free(tasks);
    }

    tasks = other.tasks;
    watchdogInitialized = other.watchdogInitialized;
    watchdogTimeoutSec = other.watchdogTimeoutSec;
    taskCount = other.taskCount;
    totalRunTime = other.totalRunTime;
    freeHeap = other.freeHeap;
    minFreeHeap = other.minFreeHeap;
    freePsram = other.freePsram;
    actualCount = other.actualCount;
    allocationFailed = other.allocationFailed;

    other.tasks = nullptr;
    other.actualCount = 0;
    other.taskCount = 0;
    other.totalRunTime = 0;
    other.allocationFailed = false;
    return *this;
}

SystemNetworkSnapshot::SystemNetworkSnapshot() {
    wifi = new WiFiConnectivityDiagnostics();
}

SystemNetworkSnapshot::~SystemNetworkSnapshot() {
    delete wifi;
    wifi = nullptr;
}

SystemNetworkSnapshot::SystemNetworkSnapshot(SystemNetworkSnapshot&& other) noexcept {
    *this = std::move(other);
}

SystemNetworkSnapshot& SystemNetworkSnapshot::operator=(SystemNetworkSnapshot&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    delete wifi;
    available = other.available;
    wifi = other.wifi;
    http = other.http;

    other.available = false;
    other.wifi = nullptr;
    return *this;
}

SystemInfoSnapshot buildSystemInfoSnapshot(const SystemApiInfoDeps& deps) {
    SystemInfoSnapshot snapshot{};

    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);

    snapshot.firmwareVersion = APP_VERSION;
    snapshot.firmwareName = APP_NAME;
    snapshot.buildTarget = BUILD_TARGET;
    snapshot.chipModel = ESP.getChipModel();
    snapshot.sdkVersion = ESP.getSdkVersion();
    snapshot.arduinoVersion = ESP_ARDUINO_VERSION_STR;
    snapshot.cpuFreqMhz = getCpuFrequencyMhz();
    snapshot.cpuRevision = chipInfo.revision;
    snapshot.cpuCores = chipInfo.cores;
    snapshot.flashChipSpeed = ESP.getFlashChipSpeed();
    snapshot.sketchSize = ESP.getSketchSize();
    snapshot.freeSketchSpace = ESP.getFreeSketchSpace();
    snapshot.coreTemp = temperatureRead();
    snapshot.internalFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    snapshot.internalTotal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    snapshot.internalLargest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    snapshot.internalMinFree = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    if (snapshot.internalLargest > snapshot.internalFree) {
        snapshot.internalLargest = snapshot.internalFree;
    }
    snapshot.psramSize = ESP.getPsramSize();
    snapshot.psramFree = ESP.getFreePsram();
    snapshot.fsTotalBytes = deps.getFsTotalBytes ? deps.getFsTotalBytes() : 0;
    snapshot.fsUsedBytes = deps.getFsUsedBytes ? deps.getFsUsedBytes() : 0;
    snapshot.uptimeSec = millis() / 1000;

    if (esp_flash_get_size(nullptr, &snapshot.flashSize) != ESP_OK) {
        snapshot.flashSize = 0;
    }

    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    snprintf(
        snapshot.macAddress,
        sizeof(snapshot.macAddress),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        macAddr[0],
        macAddr[1],
        macAddr[2],
        macAddr[3],
        macAddr[4],
        macAddr[5]);
    // Preserve the enum as an integer for REST/WS consumers so UI code can
    // map it directly without a string -> int round-trip.
    snapshot.resetReason = static_cast<int>(esp_reset_reason());

    return snapshot;
}

SystemTasksSnapshot buildSystemTasksSnapshot(const SystemApiTaskDeps& deps, bool includeDetails) {
    SystemTasksSnapshot snapshot{};
    snapshot.watchdogInitialized =
        deps.isWatchdogInitialized ? deps.isWatchdogInitialized() : false;
    snapshot.watchdogTimeoutSec =
        deps.getWatchdogTimeoutSec ? deps.getWatchdogTimeoutSec() : 0;
    snapshot.taskCount = uxTaskGetNumberOfTasks();
    snapshot.freeHeap =
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    snapshot.minFreeHeap =
        heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    snapshot.freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    if (!includeDetails) {
        return snapshot;
    }

    snapshot.detailsIncluded = true;

    const UBaseType_t maxToFetch = snapshot.taskCount + 5;
    const size_t arraySize = maxToFetch * sizeof(TaskStatus_t);
    snapshot.tasks = static_cast<TaskStatus_t*>(
        heap_caps_malloc(arraySize, MALLOC_CAP_SPIRAM));
    if (!snapshot.tasks) {
        snapshot.allocationFailed = true;
        return snapshot;
    }

    snapshot.actualCount =
        uxTaskGetSystemState(snapshot.tasks, maxToFetch, &snapshot.totalRunTime);
    return snapshot;
}

SystemNetworkSnapshot buildSystemNetworkSnapshot(const SystemApiNetworkDeps& deps) {
    SystemNetworkSnapshot snapshot{};
    if (!deps.wifiSettingsService || !snapshot.wifi) {
        return snapshot;
    }

    *snapshot.wifi = deps.wifiSettingsService->getConnectivityDiagnostics();
    snapshot.http = SYSTEM::HEALTH::HttpServerHealthTracker::getSnapshot();
    snapshot.available = true;
    return snapshot;
}

}  // namespace API
