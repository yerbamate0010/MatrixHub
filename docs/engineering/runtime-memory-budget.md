# Runtime Memory Budget - Podgol 6

Date: 2026-06-18

Target:
- Device: `https://192.168.0.30`
- Firmware env: `waveshare_esp32s3_matrix`
- Firmware: `MatrixHub 2.0.0`
- Report artifact: `artifacts/diagnostics/20260618T014442Z/podgol6-memory/memory_budget_report.md`
- Raw data: `artifacts/diagnostics/20260618T014442Z/podgol6-memory/memory_budget_raw.json`

## Verification

Commands run:
- `python -m py_compile scripts/tests/memory_budget_report.py scripts/tests/soak_test.py scripts/diagnostics/check_runtime_diagnostics.py`
- `/home/test/.platformio/penv/bin/pio test -e native`
- `/home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix`
- `SKIP_UI=1 /home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix -t upload`
- `python scripts/diagnostics/check_runtime_diagnostics.py --device-url https://192.168.0.30 --user admin --password admin --json`
- `python scripts/tests/memory_budget_report.py --device-url https://192.168.0.30 --user admin --password admin --output-dir artifacts/diagnostics/20260618T014442Z/podgol6-memory --stress-cycles 20 --stress-delay 0.02 --soak-duration-sec 30m --soak-interval-sec 10s`
- `python scripts/tests/soak_test.py --device-url https://192.168.0.30 --user admin --password admin --duration 1h --interval 10s --output artifacts/diagnostics/20260618T014442Z/podgol6-memory/soak_1h.csv --json`

Results:
- Native tests: 803 passed.
- Firmware build: success, RAM 24.8%, flash 81.3%.
- Upload: success to `/dev/ttyACM0`.
- Runtime diagnostics: pass over HTTPS/JWT.
- Memory budget report: pass.
- Endpoint stress: 400/400 successful requests, average latency 295.24 ms.
- Stable memory window: 151 snapshots across the 30 minute soak plus post-stress baseline.
- One hour soak: pass, 361 samples from 2026-06-18T02:41:58Z to 2026-06-18T03:41:58Z.

## One Hour Soak

| Metric | Result |
| --- | ---: |
| free heap min/med/max | 68888 / 70404 / 70432 |
| min free heap first/last | 54568 -> 54568 |
| free PSRAM min/med/max | 1350040 / 1431152 / 1431452 |
| internal largest block min/med/max | 31732 / 31732 / 31732 |
| PSRAM largest block min/med/max | 1310708 / 1376244 / 1376244 |
| worst stack watermark | 88 bytes, `ipc0` |
| WS queue drops | 0 -> 0 |
| WS heap fallbacks | 0 -> 0 |
| lock timeouts | standard 1 -> 1, recursive 0 -> 0 |
| slow lock acquires | standard 14 -> 23, recursive 0 -> 0 |
| boot count | 13 -> 13 |
| unexpected restarts | 1 -> 1 |

## Memory Budget Report Window

| Metric | Result |
| --- | ---: |
| internal free min/max/last | 70292 / 70596 / 70504 |
| internal largest block min/max/last | 31732 / 31732 / 31732 |
| PSRAM free min/max/last | 1430932 / 1432852 / 1432632 |
| WS queue drops | 0 -> 0 |
| WS heap fallbacks | 0 -> 0 |
| lock timeouts during report soak | 0/0 -> 0/0 |
| boot count during report soak | 13 -> 13 |
| unexpected restarts during report soak | 1 -> 1 |

## Findings

- Feature restarts are expected for keyboard, airmouse, jiggler, macros, USB terminal, and WiFi sensing when the persisted configuration changes the runtime ownership model. `MacroSettingsService` explicitly calls `RestartService::scheduleRestart()` when `enabled` changes so the boot-time macro/HID state is reconciled cleanly.
- BootTracker `unexpectedRestarts` increased once during feature activation, then stayed stable through the soak. The macro restart itself is planned; the remaining follow-up is to determine whether a planned-restart marker was lost or an extra reboot happened in the restart sequence.
- Worst stack HWM was 88 on `ipc0`, below the warning threshold of 256. This is a system task watermark and did not erode during the measured soak.
- Internal largest free block settled at 31732 bytes, just below the 32768 warning threshold, but stayed flat during the stable window.
- A post-soak diagnostics snapshot saw one global standard lock timeout. The one hour soak held that counter at 1 across millions of additional lock acquisitions. Current diagnostics are global only, so per-lock attribution should be added before treating this as a fixed root cause.
- Live-tail over HTTPS works, but current logs are dominated by repeated `SENSOR_MISSING` / SCD4x self-healing entries. That is a logging-quality issue for Podgol 7 and makes restart/log correlation harder once the 500-line ring buffer rolls over.

## Current Verdict

Podgol 6 memory stability gate passes for the measured 30 minute feature-pass report and the follow-up one hour full-feature soak. Remaining items are warnings for Podgol 7/9 follow-up, not evidence of active heap drift or a recurring runtime memory leak in the measured window.
