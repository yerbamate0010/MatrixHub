Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Planning](../README.md#planning)

# Roadmap

This file tracks active, actionable backlog for the current `waveshare_esp32s3_matrix` project.

Removed from this file on purpose:

- broad cross-chip strategy notes
- speculative ESP32-C6 / H2 product concepts
- design essays without an owner or acceptance criteria

Architecture rules now belong in `skills/esp32-firmware-agent/SKILL.md`, in `docs/engineering/*`, and in code comments near the affected runtime paths. This file should stay operational.

## Active backlog

### JSON key migration to `snake_case`

Goal:

- keep backend JSON keys, frontend DTOs, and `CONFIG::Keys` aligned
- stop leaking camelCase compatibility debt into new modules

#### Already consistent

- [x] AirMouse
- [x] Notifications
- [x] Power
- [x] Macros
- [x] Heartbeat
- [x] UDP

#### Still to migrate

BLE:

- [x] scanner-only cleanup completed
- [x] legacy peripheral, advertising, pairing, and passkey fields removed
- [x] backend cleanup in `src/api/ble/BleApiService.cpp`
- [x] frontend cleanup in `interface/src/routes/bluetooth/`
- [x] widget cleanup in `interface/src/routes/(dashboard)/widgets/ble/`

Compensation:

- [ ] `baseTempOffset` -> `base_temp_offset`
- [ ] `referenceCpuTemp` -> `reference_cpu_temp`
- [ ] `tempOffsetPerCpuDegree` -> `temp_offset_per_cpu_degree`
- [ ] `minTempOffset` -> `min_temp_offset`
- [ ] `maxTempOffset` -> `max_temp_offset`
- [ ] backend cleanup in `src/api/compensation/CompensationApiService.cpp`
- [ ] frontend cleanup in `interface/src/routes/settings/sensors/compensation/`
- [ ] API client cleanup in `interface/src/lib/services/api/integrations/CompensationApiService.ts`

Shelly:

- [ ] `relayIndex` -> `relay_index`
- [ ] `pollIntervalMs` -> `poll_interval_ms`
- [ ] `shellyDevices` -> `shelly_devices`
- [ ] backend cleanup in `src/api/shelly/ShellyApiService.cpp`
- [ ] frontend cleanup in `interface/src/routes/shelly/`
- [ ] API client cleanup in `interface/src/lib/services/api/integrations/ShellyApiService.ts`

Alarms:

- [ ] `cooldownSeconds` -> `cooldown_seconds`
- [ ] `notifyChannels` -> `notify_channels`
- [ ] `createdAt` -> `created_at`
- [ ] `updatedAt` -> `updated_at`
- [ ] `bleDeviceMac` -> `ble_device_mac`
- [ ] backend cleanup in `src/api/alarms/AlarmsApiService.cpp`
- [ ] backend cleanup in `src/api/alarms/utils/AlarmRulesSerializer.cpp`
- [ ] frontend cleanup in `interface/src/routes/alarms/`
- [ ] type cleanup in `interface/src/lib/types/domain/alarms.ts`

WiFi Sensing:

- [ ] `sampleIntervalMs` -> `sample_interval_ms`
- [ ] `varianceThreshold` -> `variance_threshold`
- [ ] resolve mixed snake_case / camelCase usage between settings and streaming payloads

Other:

- [ ] `udpPusher` -> `udp_pusher`

### Migration pattern

For each module:

1. update `CONFIG::Keys` in `src/config/App.h`
2. let ConfigJson use the new keys
3. remove hardcoded camelCase from API serializers
4. update TypeScript types and Svelte bindings
5. validate backend + frontend together

Notes:

- key renames can reset values loaded from older `config.json`
- RTC schema does not need a version bump for JSON key renames alone
- new modules should follow the convention already documented in `skills/esp32-firmware-agent/SKILL.md`

## Module quality audit backlog

Audit criteria:

- shared deserializer instead of duplicate parsing
- `memcmp` or equivalent structural change detection where appropriate
- consistent API responses
- no stray hardcoded JSON keys
- alignment with the current Service + ApiService + ConfigJson pattern

### Power

Status:

- largely fixed already
- keep an eye on drift between HTTP response fields, frontend expectations, and persisted keys

### Remaining audits

- [ ] Heartbeat
- [ ] Macros
- [ ] Compensation
- [ ] BLE
- [ ] Shelly
- [ ] Alarms
- [ ] WiFi Sensing

## Architecture cleanup backlog

- [ ] BLE: remove file-static state from `src/api/ble/BleApiService.cpp` and keep state on the service instance
- [ ] Notifications: either document `RtcStatefulService` as a first-class pattern in `skills/esp32-firmware-agent/SKILL.md` or simplify it away where it adds no value

## Deferred research

These are explicitly *not* active roadmap items for the current ESP32-S3 target, but may be revisited later as separate design notes:

- advanced Wi-Fi sensing based on CSI / promiscuous mode
- presence detection tied to external context or automation signals
- cross-target C6 / H2 strategy

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Planning](../README.md#planning)
