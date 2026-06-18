Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Integrations](../README.md#integrations-and-specialized-subsystems)

# Wi-Fi CSI Streaming

This document describes the current CSI runtime path in the firmware. It is intentionally narrow: the source of truth is the code under `src/wifisensing/csi/` and `src/api/wifisensing/`.

Important distinction:

- `CSI` currently provides raw channel data capture, batching, and WebSocket delivery to the UI.
- Motion detection used by automation and alarms lives in the separate RSSI-based path: `WifiSensingService` and `WifiSensingTaskRunner`.

## Current architecture

The active CSI path is built from four pieces:

1. `CsiService`
   - owns the CSI lifecycle
   - tracks active consumers (`Frontend`, `AlarmSystem`, `Boot`)
   - allocates the queue and processing task when the first consumer enables CSI
   - applies RX gain compensation and batches packets before forwarding them

2. `CsiPingSession`
   - opens a raw ICMP socket
   - sends Echo Requests to the current Wi-Fi gateway
   - uses short send/receive timeouts and non-blocking mode to avoid starving other network work

3. Wi-Fi CSI callback
   - `esp_wifi_set_csi_rx_cb(...)` registers `wifi_csi_rx_cb`
   - the callback copies `wifi_csi_info_t` into a fixed-size `CsiPacket`
   - inbound traffic is throttled to keep the stream stable and limit queue pressure

4. `WifiSensingApiService` and the frontend
   - exposes the WebSocket endpoint at `/ws/csi`
   - enables the `Frontend` CSI consumer only while a client is connected
   - broadcasts binary CSI frames to the browser
   - `interface/src/lib/features/wifisensing/csi/useCsiConnection.svelte.ts`
     decodes frames and feeds the CSI charts used by
     `interface/src/routes/wifisensing/csi/+page.svelte`

## Runtime data flow

The current flow is:

1. A WebSocket client connects to `/ws/csi`.
2. `WifiSensingApiService` marks the frontend CSI consumer as active.
3. `CsiService` enables CSI capture, allocates a PSRAM-backed queue, starts `CsiPingSession`, and launches the processing task.
4. The ESP32 sends ICMP Echo Requests to the gateway.
5. Incoming CSI frames are copied in the RX callback and pushed into `CsiDataQueue`.
6. The processing task drains the queue, applies gain compensation, batches packets, and passes them to the API broadcaster.
7. The browser receives binary frames and derives amplitude charts from interleaved I/Q bytes.

## Wire format used by `/ws/csi`

Each WebSocket payload contains one or more packet records concatenated back to
back. A single-packet payload is therefore just one record using the same binary
record layout.
Each packet record is encoded as:

- `uint32` timestamp
- `int8` RSSI
- `uint16` CSI payload length
- `uint8` gain compensation scaled by `* 10`
- `float32` `motionScore`
- `uint8` `isMotionDetected`
- raw interleaved I/Q bytes

Notes:

- `MAX_CSI_DATA_LEN` is `512` bytes in the queue/storage layer.
- The frontend converts I/Q pairs into amplitudes with `sqrt(re^2 + im^2)`.
- WebSocket payloads may contain a batch of multiple CSI packet records.
- The worker currently batches up to `MAX_CSI_BATCH_PACKETS` records and the
  API preserves that as one WebSocket payload. If a future worker ever emits a
  larger callback batch, the API chunks it back to valid wire-format batches
  rather than sending oversized frames.
- Receivers should parse records until the payload is exhausted; a partial
  trailing record is malformed and should be dropped.

## Runtime diagnostics

`GET /api/wifisensing/status` returns the RSSI sensing state and CSI runtime
metrics in a single authenticated JSON response. The schema field is
`wifisensing.status.v1`.

The RSSI fields include:

- configured `enabled`, `sample_interval_ms`, and `variance_threshold`
- task state: `running`, `active`, connected SSID/channel, and `motionDetected`
- current statistics: raw `current`, median `filtered`, min/max/avg,
  `variance`, `sampleCount`, and timestamp-derived `windowMs`

The nested `csi` object includes:

- consumer state: `active_consumer_mask`, `consumer_count`, and per-consumer
  booleans for frontend, alarm, and boot users
- queue pressure: `queue_allocated`, `queue_depth`, `queue_capacity`,
  `queue_drops_total`, and `queue_drops_last_sec`
- capture counters: `rx_frames_total`, `rx_accepted_total`,
  `rx_throttled_total`, `queued_packets_total`, and `dequeued_packets_total`
- transport counters: `packets_forwarded_total`, `batches_forwarded_total`,
  `batches_dropped_total`, `packets_per_sec`, and `batches_per_sec`
- freshness and calibration: `last_packet_ms`, `last_batch_ms`,
  `calibration_count`, `calibration_target`, and `calibration_state`
- WebSocket state: `ws_client_count` and `ws_queue_enabled`

`scripts/diagnostics/trigger_wifisensing.py` records this status before and
after toggling the feature. `tools/csi_client.py` prints the CSI status summary
when the endpoint is available and tolerates firmware builds where the endpoint
is not present.

## Current limitations

The repository still contains `src/wifisensing/csi/algo/CsiAlgorithm.*`, but that algorithm is not part of the active runtime path.

Today:

- `CsiService` sets `packet.motionScore = 0.0f`
- `CsiService` sets `packet.isMotionDetected = false`
- CSI data is therefore suitable for visualization, debugging, and future experimentation, not for current automation decisions

If you need real motion detection today, use the RSSI variance path:

- `src/wifisensing/WifiSensingService.*`
- `src/wifisensing/core/WifiSensingTaskRunner.*`

That path is the one currently wired into motion state evaluation and alarm integration.

## Memory and tasking notes

- `CsiDataQueue` is allocated dynamically when CSI becomes active.
- The processing task stack is allocated in PSRAM; the task control block remains in internal RAM.
- `CsiService` also allocates a PSRAM batch buffer for WebSocket delivery.
- CSI is turned off fully when there are no active consumers.
- Queue depth and drops are measurable at runtime through
  `/api/wifisensing/status`, which is the quickest check for whether CSI is
  saturating the ISR queue or the WebSocket transport.

## When to extend this module

Keep this document aligned with runtime reality. If CSI-side motion scoring is re-enabled in the future, the change is not complete until all three are updated:

1. `CsiService` processing path
2. `/ws/csi` payload semantics
3. this document

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Integrations](../README.md#integrations-and-specialized-subsystems)
