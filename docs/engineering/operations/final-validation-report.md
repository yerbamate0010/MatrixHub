# Final Validation Report

Date: 2026-06-19

Validated device URL:

- `https://192.168.0.30`

Configured mDNS alias:

- `https://plantcare.local`

The validation host could not resolve `plantcare.local` through mDNS during
the baseline or final audit (`getent hosts plantcare.local` returned no host
entry and `curl` could not resolve the name). The device configuration still
reports hostname `plantcare`, so the final live gate used the fixed HTTPS IP as
the authoritative target and records mDNS as an environment/network caveat.

Result: `PASS`

This report is the final Podgol 32 release validation artifact required by
`ostateczne_sprawdzenie.md`. Detailed command output and measured evidence are
kept in [final-regression-gate-2026-06-19.md](final-regression-gate-2026-06-19.md).

## Scope

The final gate covered firmware, frontend, HTTPS/JWT API, WebSocket behavior,
SDK/API contract hygiene, runtime diagnostics, smoke/stress/soak scripts, and
release documentation.

Telegram live delivery was intentionally excluded from live tests because this
bench has another ESP under notification testing. Telegram was covered by host
tests, validators, mocked UI tests, settings reads, and worker/runtime tests;
no live Telegram delivery/test-send endpoint was invoked.

## Baseline Comparison

Baseline source: `artifacts/diagnostics/20260617T182752Z/baseline/`

The baseline captured git state, module inventory, endpoint map, native tests,
frontend checks/tests, and device endpoint snapshots for:

- `/api/system/info`
- `/api/system/tasks?details=1`
- `/api/system/network`
- `/api/config`
- `/rest/power/status`
- `/api/ble/status`
- `/api/alarms/rules?includeStatus=1`
- `/api/matrix/settings`
- `/api/wifisensing/config`
- `/api/notifications/settings`
- `/api/macros/status`
- `/api/logs`

Final comparison:

- Device remained online after the final soak.
- Final runtime diagnostics passed on all 6 diagnostic endpoint groups.
- Final stress and soak showed `boot_count_delta = 0` and
  `unexpected_restarts_delta = 0`.
- Final soak preserved `min_free_heap` at `64656 -> 64656 B`.
- Final soak preserved internal largest block and PSRAM largest block.
- Final WebSocket queue drops remained `0 -> 0`.
- Final lock timeout counters did not grow:
  standard `0 -> 0`, recursive `2 -> 2`.
- mDNS resolver behavior matched the baseline: hostname configured on-device,
  but `plantcare.local` was not resolvable from the validation host.

## Final Gate Evidence

| Requirement | Evidence | Result |
| --- | --- | --- |
| Firmware release build passes | `pio run -e waveshare_esp32s3_matrix` | PASS |
| Firmware fits partition | RAM 81,472 / 327,680 B, flash 2,864,043 / 3,502,080 B | PASS |
| Fast backend build passes | `SKIP_UI=1 pio run -e waveshare_esp32s3_matrix` | PASS |
| Native host tests pass | `pio test -e native`, 854/854 | PASS |
| Frontend lint passes | `npm run lint` | PASS |
| Frontend type checks pass | `npm run check`, 0 errors, 0 warnings | PASS |
| Frontend unit/component tests pass | `npm run test:run`, 130 files / 578 tests | PASS |
| Frontend production build passes | `npm run build` | PASS |
| Frontend size budget passes | Client gzip 320.31 KB / 400 KB, CSS gzip 28.05 KB / 50 KB, total 1.26 MB / 1.5 MB | PASS |
| E2E passes against real HTTPS device | `DEVICE_URL=https://192.168.0.30 ... npm run test:e2e`, 33/33 | PASS |
| Fixed HTTPS IP remains authoritative test target | `https://192.168.0.30` smoke/stress/soak/runtime diagnostics | PASS |
| mDNS alias documented with resolver caveat | Baseline and final host resolver checks could not resolve `plantcare.local` | CAVEAT |
| Read-only device smoke passes | `device-smoke-192-168-0-30-20260619T071714Z.md`, 23/23 | PASS |
| Safe-writes device smoke passes | `device-smoke-192-168-0-30-20260619T071730Z.md`, 44/44 | PASS |
| Stress passes without reset/WDT | `stress-192-168-0-30-20260619T072602Z.md`, 2734/2734 | PASS |
| 1h soak passes without reset/WDT | `soak-192-168-0-30-20260619T082613Z.md`, 359 samples | PASS |
| Runtime diagnostics remain healthy after soak | `check_runtime_diagnostics.py`, 6 endpoints healthy | PASS |
| API contract is documented and verified | `docs/engineering/api-contract.md`, `scripts/contract/verify_api_contract.py` | PASS |
| Operations docs are current | `docs/engineering/operations/testing.md`, `runtime-diagnostics.md`, `security_hardening.md` | PASS |

## Stress Summary

Artifact: `artifacts/stress/stress-192-168-0-30-20260619T072602Z.md`

- Checks: 2734/2734
- Failures: 0
- Average latency: 173.08 ms
- P95 latency: 909.52 ms
- Max latency: 5536.47 ms
- Heap drift: default free -1012 B, internal free -1052 B, PSRAM free +40 B
- Largest block drift: 0 B internal, 0 B PSRAM
- WS queue drops delta: 0
- Reset delta: boot 0, unexpected 0
- Lock timeout delta: standard 0, recursive 0

## Soak Summary

Artifact: `artifacts/soak/soak-192-168-0-30-20260619T082613Z.md`

- Duration: 1h
- Samples: 359
- Request count: 1436
- Failed sample requests: 0
- Free heap min/median/max: 78088 / 78972 / 79068 B
- Min free heap drift: 64656 -> 64656 B
- Free PSRAM min/median/max: 1492352 / 1502016 / 1502088 B
- Internal largest block min/median/max: 31732 / 31732 / 31732 B
- PSRAM largest block min/median/max: 1409012 / 1409012 / 1409012 B
- Worst stack watermark: 80 B, task `ipc0`
- WS queue drops delta: 0
- WS heap fallbacks delta: 0
- Reset delta: boot 0, unexpected 0
- Lock contention delta: standard timeouts 0, recursive timeouts 0,
  standard slow acquires 0, recursive slow acquires 0

## Residual Release Notes

- Vite still emits a large chunk warning, but the enforced size budget passes.
- The lowest observed task stack watermark is `80 B` on `ipc0`; it was stable
  during stress/soak and remains visible in diagnostics.
- Soak latency tails are high while verbose logging and diagnostics polling are
  active, but this did not correlate with request failures, resets, WebSocket
  drops, heap drift, or lock timeout growth.

## Final Decision

MatrixHub satisfies the commercial-grade validation gate described in
`ostateczne_sprawdzenie.md` for this device run. No push was performed.
