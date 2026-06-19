# CSI alarm detection path

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Integrations](../README.md#integrations-and-specialized-subsystems)

## Decision

This file is a historical design note plus a remaining production-confidence
checklist. The firmware now implements a CSI alarm source named
`wifi_csi_motion`.

The current firmware streams CSI for visualization and diagnostics, and
`CsiBandMotionDetector` also produces a confirmed motion boolean for the alarm
engine. RSSI variance remains the separate alarm source `wifi_motion`.

The remaining risk is not that the runtime path is missing. The remaining risk
is field confidence: the defaults still need real empty-room, motion, reconnect,
and noisy-environment captures from representative deployments before claiming
commercial-grade false-positive behavior.

## Non-goals for the current release

- Do not alarm on raw CSI amplitude.
- Do not reuse `/ws/csi` visualization fields as alarm truth.
- Do not expose raw CSI amplitudes or `motionScore` as alarm truth.
- Do not merge CSI alarm semantics into the RSSI `wifi_motion` source.
- Do not remove lifecycle ownership, diagnostics, or power/memory visibility
  from the explicit `AlarmSystem` CSI consumer.

## Required production signal

The production alarm source is named `wifi_csi_motion`. It represents the
confirmed detector boolean as `0/1`. The CSI score and confidence remain
diagnostic fields in status and the `/ws/csi` stream.

Minimum signal pipeline:

1. Decode each CSI packet into I/Q pairs.
2. Convert pairs to energy or amplitude per subcarrier.
3. Reject malformed or empty frames, and suppress broad reconnect/gain-like
   disturbances before they can become confirmed motion.
4. Warm up after CSI enable, config change, width change, or manual calibration.
5. Calibrate an adaptive baseline from a quiet window.
6. Estimate noise floor from baseline dispersion.
7. Compute a selected-band score from the top-K per-subcarrier z-scores.
8. Apply hysteresis with separate enter and exit thresholds.
9. Require debounce/min-duration before confirmation.
10. Apply clear hold after motion stops.
11. Emit confidence and state, not only a boolean.

Minimum runtime states:

- `disabled`
- `needs_configuration`
- `calibrating`
- `monitoring`
- `motion_candidate`
- `motion_confirmed`
- `noisy_environment`
- `unavailable`

## Baseline and thresholds

The alarm path must learn baseline from device-local data because CSI amplitude
depends on AP, channel, antenna orientation, RSSI, gain state, room geometry, and
nearby transmitters.

Current default parameters:

- calibration frames: 150
- top-K carriers: 8
- medium sensitivity enter threshold: 6.0
- medium sensitivity clear threshold: 3.0
- low sensitivity enter/clear: 8.0 / 4.0
- high sensitivity enter/clear: 4.5 / 2.2
- minimum confirmed duration: 1200 ms
- clear hold: 2500 ms
- minimum noise floor: 4.0
- minimum energy: 4.0
- noisy score threshold: 80.0

These are conservative defaults. They still need harness-driven tuning from
captures before being treated as universal production constants.

## Required datasets

Before broad deployment, collect and keep uncommitted raw captures under
`artifacts/diagnostics/<timestamp>/csi-alarm/`:

- empty room, 30 minutes
- empty room with normal AP/background network traffic, 30 minutes
- one person walking through field, 10 repetitions
- person sitting still near the device, 10 minutes
- door open/close or fan/HVAC noise, 10 repetitions
- WiFi reconnect during CSI capture
- CSI frontend connect/disconnect loop
- BLE/USB/macro activity running while CSI captures

The acceptance metric is false-positive resistance first. It is better to miss a
weak CSI event than to trigger alarms from ambient noise.

## Harness

`scripts/sensing_analysis/csi_alarm_harness.py` is an offline reference harness.
It can:

- generate synthetic quiet, motion, and noisy sequences,
- read JSONL records with CSI payloads,
- run a conservative state machine,
- produce a JSON summary,
- fail when an expected quiet or motion scenario does not match.

Example commands:

```bash
python scripts/sensing_analysis/csi_alarm_harness.py --synthetic quiet --expect quiet
python scripts/sensing_analysis/csi_alarm_harness.py --synthetic motion --expect motion
python scripts/sensing_analysis/csi_alarm_harness.py --synthetic noisy --expect noisy
python scripts/sensing_analysis/csi_alarm_harness.py --input artifacts/diagnostics/csi.jsonl --expect quiet
```

Supported JSONL input shape:

```json
{"timestamp_ms": 1234, "rssi": -65, "payload": [1, 2, -3, 4]}
{"timestamp_ms": 1334, "rssi": -65, "payload_hex": "0102fd04"}
```

Each payload is interleaved signed I/Q bytes. The harness does not need live
device credentials and should be safe for CI.

## Remaining production-confidence gates

The runtime path exists. Treat these as the gates for claiming production-grade
false-positive resistance:

1. Native C++ algorithm tests cover quiet, motion, noisy, reconnect/gain jump,
   malformed, and cooldown scenarios.
2. The offline harness passes against real captures from the required datasets.
3. `/api/wifisensing/status` exposes CSI alarm state, score, confidence, noisy
   flag, carrier counts, and `last_reset_reason`.
4. Alarm evaluator receives only the boolean-like `wifi_csi_motion` signal.
   Score remains diagnostic.
5. Device smoke verifies that enabling a CSI alarm consumer keeps CSI lifecycle
   balanced and does not leak queue, task, or WebSocket resources.

## Current firmware shape

Implementation layout:

- `src/wifisensing/csi/algo/CsiBandMotionDetector.{h,cpp}`
- `test/test_csi_band_motion_detector/`
- `WIFISENSING::CSI::CsiMotionSnapshot`
- `CsiService::getMotionSnapshot()`
- alarm input field `wifiCsiMotion`
- alarm source string `wifi_csi_motion`

The detector should be fed by the CSI processing task after gain compensation
and before WebSocket serialization. It must not allocate on the packet path and
must not call alarm notification code directly.

## Current status

Runtime implementation is active:

- CSI visualization remains active,
- RSSI `wifi_motion` alarms remain unchanged,
- CSI alarm rules use the separate source `wifi_csi_motion`,
- `AlarmService` consumes only a boolean-like CSI motion value,
- `motionScore`, confidence, noisy state, and reset reason remain diagnostics,
- the offline harness and real captures remain the next confidence step.

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Integrations](../README.md#integrations-and-specialized-subsystems)
