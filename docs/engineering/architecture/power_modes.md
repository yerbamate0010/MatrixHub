Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Architecture](../README.md#runtime-and-architecture)

# Power Modes and Sleep Flow

This document describes the current power subsystem in the repo.

## Current model

Power behavior is driven by `POWER::PowerManager` and consists of:

- inactivity-based auto-sleep
- manual sleep requests
- maintenance sleeps such as hygiene sleep
- wake-source handling
- RTC-backed activity restoration after wake

The main code paths are:

- `src/system/power/PowerManager.*`
- `src/system/power/PowerActivityTracker.*`
- `src/system/power/PowerSleepController.*`
- `src/system/power/PowerWakeController.*`
- `src/system/shutdown/ShutdownSequence.cpp`

## Persistent settings

### Runtime source of truth

Power settings live in RTC config:

- `RTC::ConfigStore::power`

JSON-facing keys are:

- `sleep_enabled`
- `inactivity_timeout_ms`
- `grace_after_boot_ms`

Those are loaded/saved through `src/config/json/PowerSettingsJson.cpp`.

### NVS backup

`PowerSettings` still keeps a Preferences backup in namespace `power_cfg`:

- `inact_ms`
- `grace_ms`
- `sleep_en`

These are implementation details for backup compatibility. New docs and frontend code should use the JSON keys above.

## HTTP endpoints

### `GET /rest/power/status`

Authenticated endpoint returning current power state, including:

- `wake_reason`
- `wake_cause_raw`
- `wake_gpio_mask`
- `wake_ext1_mask`
- `sleep_requested`
- `sleep_eta_ms`
- `sleep_enabled`
- `inactivity_timeout_ms`
- `grace_after_boot_ms`
- `wake_interval_ms`
- `last_activity_ms`
- `uptime_ms`

### `PUT /rest/power/config`

Admin endpoint for updating:

- `sleep_enabled`
- `inactivity_timeout_ms`
- `grace_after_boot_ms`

Changes are applied immediately through `PowerManager` and persisted to RTC plus NVS backup.

### `POST /rest/power/hygieneSleep`

Admin endpoint that triggers a short maintenance sleep used for heap hygiene. Current implementation sets wake interval to `100ms` before requesting sleep.

### `POST /rest/sleep`

Admin endpoint that requests normal deep sleep through `PowerManager` instead of relying on the framework default handler.

## Activity tracking

`PowerActivityTracker` is responsible for deciding when inactivity sleep should happen.

Current rules:

- last activity and boot timestamps are restored from RTC when possible
- AP clients count as activity, so the device should not sleep while someone is connected to SoftAP
- the grace period is respected after boot
- inactivity sleep only happens if `sleep_enabled` is true

If inactivity exceeds the configured timeout, the tracker requests:

- `requestSleep("inactivity")`

## Sleep path

Once sleep is requested:

1. `PowerSleepController` waits for the requested delay, if any
2. `SleepService::executeSleepCallbacks()` is called
3. the pre-sleep hook runs `ShutdownSequence::execute()`
4. wake sources are configured by `PowerWakeController`
5. `RTC::prepareForSleep()` updates RTC CRCs and snapshots
6. the firmware enters deep sleep with `esp_deep_sleep_start()`

## Wake interval

Normal wake interval comes from `POWER::WAKE_INTERVAL_MS`.

Specific paths can override it temporarily, for example:

- hygiene sleep uses `100ms`
- thermal emergency cooling can set a longer interval before requesting sleep

## Frontend implications

Frontend code should treat `GET /rest/power/status` as the authoritative status endpoint and use:

- `sleep_enabled`
- `inactivity_timeout_ms`
- `grace_after_boot_ms`
- `wake_reason`
- `sleep_eta_ms`

Do not build new UI against backup-only names such as `grace_ms`.

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Architecture](../README.md#runtime-and-architecture)
