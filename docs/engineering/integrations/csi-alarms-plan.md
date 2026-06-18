# CSI alarm detection path

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Integrations](../README.md#integrations-and-specialized-subsystems)

## Decision

CSI is not enabled as a production alarm source yet.

The current firmware streams CSI for visualization and diagnostics, while alarm
source `wifi_motion` is RSSI variance. The archived CSI algorithm in
`src/features/sensing/csi/archive/` is a useful reference, but it is disabled and
does not yet have enough false-positive evidence for commercial alarm behavior.

This podgol therefore defines the production path and adds an offline harness.
Runtime alarm integration must wait until the harness has real empty-room,
motion, reconnect, and noisy-environment captures.

## Non-goals for the current release

- Do not alarm on raw CSI amplitude.
- Do not reuse `/ws/csi` visualization fields as alarm truth.
- Do not expose `csi_motion_score` in the rule UI until false-positive tests pass.
- Do not keep CSI enabled only because an alarm rule exists unless the service
  has explicit lifecycle ownership, diagnostics, and power/memory budget.

## Required production signal

A production CSI alarm source should be named `csi_motion_score` or a similarly
explicit contract name. It must represent a normalized confidence-bearing score,
not raw amplitude or single-frame variance.

Minimum signal pipeline:

1. Decode each CSI packet into I/Q pairs.
2. Convert pairs to energy or amplitude per subcarrier.
3. Reject malformed, empty, reconnect-period, and gain-reset frames.
4. Warm up after CSI enable or WiFi reconnect.
5. Calibrate an adaptive baseline from a quiet window.
6. Estimate noise floor from baseline dispersion.
7. Compute a rolling temporal score over a fixed window.
8. Apply hysteresis with separate enter and exit thresholds.
9. Require debounce/min-duration before confirmation.
10. Apply cooldown after clear.
11. Emit confidence and state, not only a boolean.

Minimum runtime states:

- `idle`
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

Initial suggested parameters for offline falsification:

- warmup frames: 30
- calibration frames: 150
- score window: 40 frames
- noise floor multiplier: 6.0 for motion candidate
- clear multiplier: 3.0
- minimum confirmed duration: 1500 ms
- noisy-environment threshold: baseline dispersion above expected quiet range
- reconnect holdoff: at least 5000 ms after WiFi/CSI reset

These are not production constants. They are starting points for harness-driven
experiments and must be tuned from captures.

## Required datasets

Before enabling a firmware alarm source, collect and keep uncommitted raw
captures under `artifacts/diagnostics/<timestamp>/csi-alarm/`:

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

## Runtime integration gates

Do not add `csi_motion_score` to `AlarmSource` until all gates pass:

1. Native C++ algorithm tests cover quiet, motion, noisy, reconnect, malformed,
   and cooldown scenarios.
2. The offline harness passes against real captures from the required datasets.
3. `/api/wifisensing/status` exposes CSI alarm state, score, confidence,
   baseline age, noisy flag, and last reset reason.
4. Alarm evaluator receives CSI score only when state is `monitoring` or
   `motion_confirmed`; all other states must evaluate as unavailable.
5. Frontend alarm rule UI labels the source as experimental unless real-world
   false-positive data proves otherwise.
6. Device smoke verifies that enabling a CSI alarm consumer keeps CSI lifecycle
   balanced and does not leak queue, task, or WebSocket resources.

## Future firmware shape

Recommended implementation layout:

- `src/wifisensing/csi/algo/CsiMotionDetector.{h,cpp}`
- `test/test_csi_motion_detector/`
- `WIFISENSING::CSI::CsiMotionSnapshot`
- `CsiService::getMotionSnapshot()`
- alarm input field `csiMotionScore`
- alarm source string `csi_motion_score`

The detector should be fed by the CSI processing task after gain compensation
and before WebSocket serialization. It must not allocate on the packet path and
must not call alarm notification code directly.

## Current status

Deferral is intentional and release-safe:

- current CSI visualization remains active,
- current RSSI `wifi_motion` alarms remain unchanged,
- no new alarm source is exposed,
- no user-facing false-positive path is introduced,
- an offline harness and integration gates now define the next implementation
  step.

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Integrations](../README.md#integrations-and-specialized-subsystems)
