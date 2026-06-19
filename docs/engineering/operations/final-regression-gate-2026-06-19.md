# Final Regression Gate - 2026-06-19

Target device: `https://192.168.0.30`

Result: `PASS`

Telegram live delivery was intentionally excluded from this gate because the shared test environment has another ESP under notification testing. The Telegram configuration was only read through settings endpoints and mocked UI tests; no live Telegram send/test endpoint was invoked.

## Code Changes From This Gate

- Stabilized dashboard E2E by replacing `networkidle` waiting with a visible dashboard widget assertion. The dashboard keeps live sockets/polling active, so `networkidle` is not a reliable readiness signal.
- Split the large UX/a11y route smoke test into one Playwright test per route and reduced route-render timeouts from 60s to 20s. This keeps failure reports precise and prevents one long opaque test from hiding progress.

## Build And Host Gates

| Gate | Result | Notes |
| --- | --- | --- |
| `pio test -e native` | PASS | 854/854 test cases |
| `pio run -e waveshare_esp32s3_matrix` | PASS | RAM 81,472 / 327,680 bytes (24.9%), flash 2,864,043 / 3,502,080 bytes (81.8%) |
| `SKIP_UI=1 pio run -e waveshare_esp32s3_matrix` | PASS | Same firmware size, UI generation skipped |
| `npm run lint` | PASS | Re-run after E2E test stabilization |
| `npm run check` | PASS | 0 errors, 0 warnings |
| `npm run test:run` | PASS | 130 files, 578 tests |
| `npm run build` | PASS | Vite large chunk warning only |
| `npm run size` | PASS | Client gzip 320.31 KB / 400 KB, CSS gzip 28.05 KB / 50 KB, total 1.26 MB / 1.5 MB |

## Frontend E2E

| Gate | Result | Notes |
| --- | --- | --- |
| `DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e` | PASS | 33/33 tests, 10.3 min |

Observed during E2E:

- Vite WebSocket proxy emitted intermittent `EPIPE` / `ECONNRESET` messages while navigating live pages.
- Follow-up device stress, soak, and final runtime diagnostics showed no boot count delta, no unexpected restart delta, no WebSocket queue drop delta, and healthy diagnostics endpoints.

## Device Smoke

| Gate | Result | Artifact |
| --- | --- | --- |
| Read-only smoke | PASS, 23/23 | `artifacts/device-smoke/device-smoke-192-168-0-30-20260619T071714Z.md` |
| Safe-writes smoke | PASS, 44/44 | `artifacts/device-smoke/device-smoke-192-168-0-30-20260619T071730Z.md` |

## Stress

Artifact: `artifacts/stress/stress-192-168-0-30-20260619T072602Z.md`

- Result: `PASS`
- Checks: 2734/2734
- Failures: 0
- Average latency: 173.08 ms
- P95 latency: 909.52 ms
- Max latency: 5536.47 ms
- Heap drift: default free -1012 B, internal free -1052 B, PSRAM free +40 B
- Internal largest block drift: 0 B
- PSRAM largest block drift: 0 B
- WS queue drops delta: 0
- Reset detection: boot count delta 0, unexpected restart delta 0
- Lock contention delta: standard timeouts 0, recursive timeouts 0, standard slow acquires 0, recursive slow acquires 0
- Scenarios included endpoint stress, invalid payload flood, WebSocket reconnect/multi-client, BLE scan loop, CSI connect loop, macro run/stop loop, Wi-Fi recovery probe, and file upload/download loop.

## Soak

Artifact: `artifacts/soak/soak-192-168-0-30-20260619T082613Z.md`

- Result: `PASS`
- Duration: 1h
- Samples: 359
- Request count: 1436
- Failed sample requests: 0
- Average latency: 4526.86 ms
- P95 latency: 7308.71 ms
- Max latency: 21800.87 ms
- Free heap min/median/max: 78088 / 78972 / 79068 B
- Min free heap drift: 64656 -> 64656 B, +0 B
- Free PSRAM min/median/max: 1492352 / 1502016 / 1502088 B
- Internal largest block min/median/max: 31732 / 31732 / 31732 B
- PSRAM largest block min/median/max: 1409012 / 1409012 / 1409012 B
- Worst stack watermark: 80 B, task `ipc0`
- WS queue drops delta: 0
- WS heap fallbacks delta: 0
- Reset detection: boot count delta 0, unexpected restart delta 0
- Lock contention delta: standard timeouts 0, recursive timeouts 0, standard slow acquires 0, recursive slow acquires 0

## Final Runtime Diagnostics

Command:

```sh
python scripts/diagnostics/check_runtime_diagnostics.py --device-url https://192.168.0.30 --username admin --password admin
```

Result: `OK runtime diagnostics: 6 endpoint(s) healthy at https://192.168.0.30`

## Residual Notes

- The Vite client bundle still emits the standard large chunk warning, but the repository size budget passes.
- The worst reported stack watermark during stress/soak is `80` bytes on `ipc0`. This did not degrade during the run, but it should remain visible in release diagnostics because it is the lowest task watermark.
- Soak latency tails are high under verbose logging and live diagnostics polling. They did not correlate with request failures, resets, WebSocket drops, heap drift, or lock timeout growth in this gate.
