---
description: Checklist and guide for creating a new module (Feature)
---

# How to Create a New Module

Follow this checklist to add a new feature (e.g., `Thermostat`) following the exact project architecture.

## Core Architectural Principles (SOLID)

Before writing any code, ensure your design follows these principles:

1.  **Single Responsibility Principle (SRP)**:
    *   **Service**: Pure business logic and state management (e.g., `ThermostatService`).
    *   **API Service**: HTTP request handling, JSON parsing/validation (e.g., `ThermostatApiService`).
    *   **Settings Service**: Configuration persistence (e.g., `RtcConfig`).
    *   *Do not mix these concerns.*

2.  **Dependency Inversion Principle (DIP)**:
    *   **NO Singletons**: Do not use `static getInstance()` or global variables.
    *   **Dependency Injection**: All dependencies must be passed via the **constructor**.
    *   *Why?* Makes testing easier (mocking) and clearly documents what a module needs to function.

3.  **Interface Segregation**:
    *   Only include headers you strictly need in `.h` files. Use forward declarations (`class AlarmService;`) where possible to reduce compilation dependencies.

---

## 1. Define Persistent Data (RTC)
*   **Goal**: Define POD (Plain Old Data) structure that survives deep sleep.
*   **Location**: `src/system/rtc/types/`

- [ ] Create `src/system/rtc/types/RtcThermostatTypes.h`.
- [ ] Define struct with `__attribute__((packed))`:
    ```cpp
    namespace RTC {
    struct __attribute__((packed)) ThermostatData {
        bool enabled = false;
        float targetTemp = 21.5f;
        char name[32] = "Living Room"; // Fixed size char array, NO String!
        uint8_t mode = 0;              // 0=Off, 1=Heat, 2=Cool
    }; 
    }
    ```
    > ⚠️ **Packed struct gotcha**: Members of packed structs cannot be bound by reference (`T&`). When reading from JSON into packed fields, always use a **temp variable**:
    > ```cpp
    > float v = constrain(obj["target_temp"].as<float>(), MIN, MAX);
    > data.targetTemp = v;  // assignment from temp, NOT direct binding
    > ```
- [ ] Open `src/system/rtc/RtcConfig.h`:
    - [ ] Include your new header.
    - [ ] Add `ThermostatData thermostat;` to `ConfigStore` struct.
    - [ ] **Critical**: Increment `kSchemaVersion` in `RtcConfig.h`.
- [ ] Open `src/system/rtc/RtcConfig.cpp`:
    - [ ] Initialize default values in `initDefaults()`:
        ```cpp
        store->thermostat.enabled = false;
        strlcpy(store->thermostat.name, "Default", sizeof(store->thermostat.name));
        ```

## 2. Implement JSON Serialization
*   **Goal**: Map JSON <-> RTC Struct for `ConfigManager`.
*   **Location**: `src/config/json/`

### JSON Key Naming Convention
> **All JSON keys MUST use `snake_case`** (e.g., `"sensitivity_x"`, `"interval_ms"`).
> This applies to: `Keys::` constants in `App.h`, config file keys, API request/response keys, and frontend TypeScript types.
> **All three layers (config file, API, frontend) must use the same key names.**

- [ ] Define key constants in `src/config/App.h`:
    ```cpp
    // Thermostat
    constexpr const char* kThermostat = "thermostat";
    constexpr const char* kTargetTemp = "target_temp";   // snake_case!
    constexpr const char* kHeatMode = "heat_mode";       // snake_case!
    ```
- [ ] Create `src/config/json/ThermostatConfigJson.h` & `.cpp`.
- [ ] **Header**: Declare `void loadThermostat(JsonObject& obj);`, `void saveThermostat(JsonObject& obj);`, and `void deserializeThermostat(JsonObject& obj, RTC::ThermostatData& data);`.
- [ ] **Cpp Implementation**:
    - [ ] `deserializeThermostat`: **Shared validation logic** used by both file loading AND API updates. Updates only fields present in JSON (partial-update safe). Use `Keys::` constants — never hardcode key strings.
        ```cpp
        void deserializeThermostat(JsonObject& obj, RTC::ThermostatData& data) {
            if (obj[Keys::kEnabled].is<bool>()) data.enabled = obj[Keys::kEnabled].as<bool>();
            if (obj[Keys::kTargetTemp].is<float>()) {
                float v = constrain(obj[Keys::kTargetTemp].as<float>(), MIN_TEMP, MAX_TEMP);
                data.targetTemp = v;  // use temp variable for packed structs
            }
        }
        ```
    - [ ] `loadThermostat`: Thin wrapper calling `deserializeThermostat`.
    - [ ] `saveThermostat`: Read from `RTC::getConfig().thermostat`, write to JSON using `Keys::` constants.
- [ ] Open `src/config/ConfigManager.cpp`:
    - [ ] Add `#include "json/ThermostatConfigJson.h"`.
    - [ ] Add loader to `load()`: `if (doc["thermostat"]) JSON::loadThermostat(doc["thermostat"]);`
    - [ ] Add saver to `save()`: `JSON::saveThermostat(doc["thermostat"].to<JsonObject>());`

## 3. Implement Business Logic (Service)
- [ ] Create folder: `src/thermostat/`
- [ ] Create `ThermostatService.h` and `.cpp`.
- [ ] **Coding Style**:
    - [ ] Namespace: `THERMOSTAT` (Uppercase).
    - [ ] Class: `ThermostatService` (PascalCase).
    - [ ] Members: `_enabled`, `_lastUpdate` (Underscore prefix).
    - [ ] **Dependency Injection**:
    - [ ] Declare dependencies in the constructor: `ThermostatService(ALARMS::AlarmService* alarmService);`
    - [ ] Store them as private members: `ALARMS::AlarmService* _alarmService;`
    - [ ] **Rules**:
        - [ ] NEVER use `Service::instance()` inside methods.
        - [ ] NEVER create dependencies inside the constructor (use `new`).
- [ ] **No static getInstance()**: Rely entirely on `ServiceRegistry` injection.
- [ ] **Logging**: Add `#define LOG_TAG "Thermostat"` in .cpp file.
- [ ] **Task Definition** (if needed):
    - [ ] Open `src/config/System.h`.
    - [ ] Add `STACK_THERMOSTAT`, `PRIO_THERMOSTAT`, `CORE_THERMOSTAT` in `CONFIG::TASKS`.
    - [ ] Use these constants in `xTaskCreate` / `xTaskCreateStatic`.
- [ ] Implement `begin()` and core logic.
- [ ] **Rule**: Do NOT depend on `PsychicHttp` or `API` classes. Keep it pure logic.
- [ ] **Runtime config propagation**: If the service has a FreeRTOS task loop, implement `applySettings()` that:
    1. Reads new config from `RTC::getConfig()`
    2. Sets a flag (e.g., `_configChanged = true`) — **for ALL config changes, not just some**
    3. Task loop checks the flag and calls `updateComponentConfigs()` to propagate to processors/handlers
    > ⚠️ **Common bug**: Only setting the flag for *some* config changes (e.g., filter params but not sensitivity). Always set `_configChanged = true` unconditionally in `applySettings()`.

## 4. Implement API (Controller)
- [ ] Create folder: `src/api/thermostat/`
- [ ] Create `ThermostatApiService.h` and `.cpp`.
- [ ] Inherit from `BaseApiService`.
- [ ] Inject `ThermostatService*` in constructor.
- [ ] Register routes in `begin()` (e.g., `server->on(...)`).
- [ ] Use `wrapAdmin(...)` for secured endpoints.
- [ ] **DRY**: For PUT endpoints, call `CONFIG::JSON::deserializeThermostat(obj, data)` from ConfigJson — do NOT duplicate validation logic in the API layer.
- [ ] **JSON keys**: Use the same `snake_case` keys as defined in `Keys::`. API, config file, and frontend MUST all use identical key names.
- [ ] **Change detection**: Use `memcmp` to detect if config actually changed before writing:
    ```cpp
    auto currentCfg = RTC::getConfig().thermostat;
    auto nextCfg = currentCfg;  // copy
    CONFIG::JSON::deserializeThermostat(obj, nextCfg);
    bool changed = (memcmp(&currentCfg, &nextCfg, sizeof(RTC::ThermostatData)) != 0);
    if (!changed) return Response::success(request, 200, "no_changes");
    ```
- [ ] **API response conventions**:
    - `200` + `"no_changes"` — input valid but nothing changed
    - `200` + `"ok"` — config updated, no restart needed
    - `200` + `"restart_required"` — config updated, service restart pending
    - `400` + `"input/invalid_json"` — malformed JSON
- [ ] **Safe JSON Generation**: Use `utils::JsonResponseWriter` for GET responses to avoid buffer overflows and manual string formatting.

## 5. Register in System
*   **Location**: `src/system/ServiceRegistry`

- [ ] Open `src/system/ServiceRegistry.h`:
    - [ ] Add `ThermostatService* _thermostatService = nullptr;` (Business Logic).
    - [ ] Add `SYSTEM::PsramStaticService<API::ThermostatApiService> _thermostatApi;` (API Controller).
- [ ] Open `src/system/ServiceRegistry.cpp`:
    1.  Include headers.
    2.  **Phase 1 (Hardware/Logic)**: In `begin()`, initialize Service:
        ```cpp
        _thermostatService = new ThermostatService(); 
        _thermostatService->begin();
        ```
    3.  **Phase 3 (API)**: Initialize API Service:
        ```cpp
        _thermostatApi.init(_server, _framework->getSecurity(), _thermostatService);
        _thermostatApi->begin();
        ```
    4.  **Cleanup**: In destructor, `delete _thermostatService;`. (API is auto-cleaned by PsramStaticService).

## 6. Memory Optimization (Critical)
> **Golden Rule**: **Aggressively offload everything possible to PSRAM.** Internal DRAM is extremely limited and should be treated as a scarce resource reserved only for DMA, ISRs, and critical real-time tasks.
- [ ] **JSON**: For API handlers, use `SYSTEM::SpiRamJsonDocument doc(4096);`.
- [ ] **PSRAM Buffers**: Use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` for large user-space buffers, HTTP responses, logs, and any data structures that don't need hardware DMA.
- [ ] **Tasks**: Use `xTaskCreateStatic` with `EXT_RAM_BSS_ATTR` buffer for stack (unless it's a critical latency-sensitive hardware task needing DRAM).
- [ ] **DMA & ISR (DRAM Exceptions)**: **NEVER** use PSRAM for buffers passed to DMA hardware (SPI, I2S, LED Matrix, unbuffered file I/O) or accessed in ISRs! Only in these specific cases, force internal memory: `heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)`.

## 7. Standard Toolbox (Don't reinvent the wheel)
*   **Logging**:
    *   Use `LOGI("Msg %d", val)` for Info or `LOGE("Error")` for Error. Avoid `Serial.print()`.
    *   `LOG_STACK()` - Prints High Water Mark (stack usage) for current task.
*   **Time**:
    *   Use `vTaskDelay(pdMS_TO_TICKS(ms))` for tasks. Avoid `delay()`.
*   **Synchronization**:
    *   `#include "system/utils/ScopeLock.h"`
    *   Use `SYSTEM::ScopeLock lock(mutex);` for RAII-style mutex locking (auto-unlock).
    *   Use `std::atomic<T>` only for 64-bit values or lock-free flags used outside of mutexes. If using `ScopeLock`, a regular variable is redundant as an atomic.
*   **Persistence (NVM)**:
    *   Use `RTC::getConfig()` for fast reads.
    *   Use `RTC::updateConfig([](auto& cfg) { ... })` for thread-safe updates.

## 8. Frontend (if applicable)
- [ ] Define TypeScript interface with **same `snake_case` keys** as backend:
    ```typescript
    interface ThermostatConfig {
        enabled: boolean;
        target_temp: number;  // matches Keys::kTargetTemp
        heat_mode: number;
    }
    ```
- [ ] Create API service in `interface/src/lib/services/api/integrations/`
- [ ] Create page/component in `interface/src/routes/`

## 9. Verify
- [ ] Run `pio test -e native` (if unit tests added).
- [ ] Run `pio run` to check compilation.
- [ ] Run `pio run --target upload` to upload and test on device.
- [ ] Test API: `curl -X GET http://<IP>/api/thermostat/status`
- [ ] Test PUT: `curl -X PUT -H 'Content-Type: application/json' -d '{"target_temp": 22.0}' http://<IP>/api/thermostat/config`
- [ ] Verify config persists after restart.
