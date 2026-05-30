Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Operations](../README.md#operations)

# Testing Guide

This repository currently uses two main test layers:

- PlatformIO native unit tests in `test/`
- SvelteKit frontend checks and tests in `interface/`

There is no `native_test` environment in the current repo. The active PlatformIO host-test environment is `env:native`.

## Native Unit Tests

Run the default host test suite:

```bash
pio test -e native
```

Run a specific suite:

```bash
pio test -e native -f "test_ble_device_type_detector"
pio test -e native -f "test_telegram_json_parsers"
```

Run multiple suites:

```bash
pio test -e native -f "test_ble_device_type_detector" -f "test_macro_api_service"
```

Verbose output:

```bash
pio test -e native -f "test_macro_logic" -v
```

Run the host suite with compiler coverage instrumentation:

```bash
pio test -e native_coverage
```

Generate a summarized coverage report on macOS Command Line Tools / Xcode toolchains:

```bash
bash scripts/tests/native_coverage.sh
```

## Current Native Environment

`platformio.ini` defines:

```ini
[env:native]
platform = native
test_framework = unity
```

Coverage-enabled host runs are available through:

```ini
[env:native_coverage]
extends = env:native
```

Host tests also rely on `test/stubs/` and `test/stubs/mocks/` include paths configured in `platformio.ini`.

`env:native_coverage` uses the same stubs and suite selection as `env:native`, but adds LLVM coverage instrumentation. `scripts/tests/native_coverage.sh` stores raw profiles under `.pio/build/native_coverage/profiles/`, merges them into `.pio/build/native_coverage/coverage/merged.profdata`, and prints a text summary with `llvm-cov`.

## Frontend Validation

Run the core frontend checks from `interface/`:

```bash
cd interface
npm run check
npm run test:run
npm run lint
```

Useful extras:

```bash
cd interface
npm run test:coverage
npm run size
npm run deps:check
```

## Frontend E2E (Playwright)

The frontend talks to the device through relative `/rest/...` and `/api/...` URLs. During local runs, Vite proxies those requests to the ESP32.

Install browsers once:

```bash
cd interface
npm run test:e2e:install
```

Run E2E tests:

```bash
cd interface
npm run test:e2e
```

Run against another device:

```bash
cd interface
DEVICE_URL=http://192.168.0.123 npm run test:e2e
```

Override credentials if needed:

```bash
cd interface
TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
```

## Test Layout

Native tests live under `test/test_<module>/`.

Examples currently in the repo:

- `test/test_ble_device_type_detector/`
- `test/test_macro_api_service/`
- `test/test_telegram_command_parser/`
- `test/test_wifi_sensing_config_json/`

Frontend tests live next to the code they validate, for example:

- `interface/src/routes/.../*.test.ts`
- `interface/src/routes/.../*.test.svelte.ts`
- `interface/src/lib/services/api/...*.test.ts`

## Adding New Native Tests

1. Create a directory like `test/test_my_module/`.
2. Add a Unity test file such as `test/test_my_module/test_my_module.cpp`.
3. Use `#include <unity.h>`.
4. Run it with:

```bash
pio test -e native -f "test_my_module"
```

## Adding New Frontend Tests

- Prefer Vitest for hooks, services, parsers, and UI logic.
- Keep tests close to the feature under `interface/src/...`.
- Use `.test.svelte.ts` when the test exercises Svelte runes/stateful hooks.

Run a focused frontend suite with Vitest:

```bash
cd interface
npm run test -- useWifiManagement
```

## Reading a Coredump

The `coredump` partition (64 KiB at `0x3E7000`) is written automatically when
the firmware panics. To decode the most recent crash into a readable backtrace:

```bash
python scripts/diagnostics/decode_coredump.py
```

This pulls the partition over USB using `esp-coredump` (bundled with the
PlatformIO penv) and resolves symbols against `.pio/build/waveshare_esp32s3_matrix/firmware.elf`
or the `build/elf/latest.elf` symlink maintained by `scripts/build/save_elf.py`.

Common options:

```bash
python scripts/diagnostics/decode_coredump.py --port /dev/cu.usbmodem101
python scripts/diagnostics/decode_coredump.py --elf build/elf/<hash>.elf
python scripts/diagnostics/decode_coredump.py --save crashlogs/2026-05-21.bin
```

Use `--save` when reporting an incident — it archives the raw partition image
and a snapshot of the matching ELF side-by-side so the trace can be re-decoded
later even after the firmware ELF rotates.

The script never writes to the device; the coredump partition is only erased
on the next panic or after an explicit re-flash.

## Troubleshooting

If a native test does not run:

1. Check that its directory exists under `test/`.
2. Check the filter name passed to `-f`.
3. Check whether the suite is listed in `test_ignore` for `env:native`.

If a native test fails to compile because it uses ESP32-only APIs:

1. Add or extend stubs in `test/stubs/`.
2. Separate logic from hardware-specific code.
3. Only add to `test_ignore` when stubbing is not worth the effort.

If frontend E2E fails immediately:

1. Verify the ESP32 is reachable on the expected `DEVICE_URL`.
2. Verify the login credentials.
3. Verify `interface/` dependencies and Playwright browsers are installed.

## Recommended Pre-Commit Validation

For backend-only changes:

```bash
pio test -e native
```

For frontend-only changes:

```bash
cd interface
npm run check
npm run test:run
```

For cross-cutting frontend/backend changes:

```bash
pio test -e native
cd interface && npm run check
cd interface && npm run test:run
```

## Frontend Refactor Smoke Checklist

Use this quick manual pass before merging a wider frontend refactor, especially
when it touches shared stores, dashboard widgets, navigation, or settings
controllers.

Recommended local gate first:

```bash
cd interface
npm run check
npm run build
```

Targeted verification areas:

1. Dashboard alarms widget
   Expected:
   - data appears from the live snapshot without a visible jump from stale REST data
   - toggling a rule updates immediately
   - if the toggle request fails, the switch returns to the previous state and an error is shown

2. Alarms page
   Expected:
   - create, edit, delete, and enable/disable all persist correctly
   - the dashboard widget and `/alarms` stay in sync after changes
   - the max rule limit is enforced and clearly messaged

3. Wi-Fi sensing page
   Expected:
   - settings only show a success toast after a real save success
   - failed saves do not show a success toast
   - enable/disable, interval, and threshold survive a reload after save

4. Shelly page and dashboard widget
   Expected:
   - add, edit, delete, and toggle all behave consistently
   - read-only users cannot mutate state
   - widget state matches the full Shelly page

5. Power settings page
   Expected:
   - invalid timeout/grace values block save
   - valid boundary values save correctly
   - blur/clamp behavior keeps the form in a valid persisted range

6. Navigation, help, and status bar
   Expected:
   - help links resolve to the correct destinations
   - admin-only items remain visible but disabled for non-admin users where intended
   - Wi-Fi CSI is disabled when STA is disconnected
   - status bar title matches the active feature route

Suggested test notes template:

```text
Build/check:
- npm run check: PASS/FAIL
- npm run build: PASS/FAIL

Manual smoke:
- Dashboard alarms: PASS/FAIL
- /alarms CRUD + toggle: PASS/FAIL
- /wifisensing save/error flow: PASS/FAIL
- /shelly page + widget sync: PASS/FAIL
- /system/power validation: PASS/FAIL
- navigation/help/statusbar: PASS/FAIL

Issues found:
- none / short bullet list
```

## Native Test Debt Reduction Plan

The host suite is healthy and broad, but some of that coverage still depends on
white-box test seams. The goal is to keep the current regression-detection
strength while steadily reducing test fragility.

Current debt themes:

- some suites still use `#define private public` to reach private state/helpers
- many host suites include production `.cpp` files directly because
  `env:native` uses `test_build_src = no`
- a few runtime wiring tests depend on large hand-written stubs because the
  production code exposes broad initialization surfaces

Reduction order:

1. Remove unnecessary `private public` usage first.
   These are the cheapest wins because they reduce coupling without changing
   production behavior.
2. Prefer narrow runtime/control seams over broad white-box access.
   Recent examples:
   - `src/shelly/ShellyRuntimeControl.*`
   - `src/system/services/ServiceRegistryOwnedApiRuntime.*`
3. When a host test needs only orchestration logic, extract a small helper file
   with forward-declared pointer dependencies instead of pulling in a large
   feature root.
4. Reserve direct `.cpp` inclusion for algorithmic code and translation units
   that would otherwise require a disproportionate stub surface.
5. Keep `env:native` green while shrinking debt incrementally.
   Avoid large rewrites of old suites unless the touched production area already
   justifies the churn.

Recommended next slices:

- reduce white-box coupling in shutdown/factory-reset lifecycle tests further by
  extracting more shutdown-only helpers behind small runtime seams
- continue decomposing `ServiceRegistry` initialization into testable helper
  files with narrow dependency sets
- keep `env:native` and this document in sync when a suite is temporarily
  excluded from the default host gate

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Operations](../README.md#operations)
