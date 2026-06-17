#!/usr/bin/env python3
"""CSI WebSocket client for MatrixHub over HTTPS/WSS."""

from __future__ import annotations

import argparse
import asyncio
import struct
import sys
import time
from pathlib import Path

import websockets

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from device_client import DeviceClient, add_common_device_args  # noqa: E402


def connect_ws(uri: str, headers: dict[str, str], ssl_context):
    try:
        return websockets.connect(uri, additional_headers=headers, ssl=ssl_context)
    except TypeError:
        return websockets.connect(uri, extra_headers=headers, ssl=ssl_context)


async def csi_client(client: DeviceClient, duration: float | None) -> int:
    uri = client.ws_url("/ws/csi")
    headers = client.ws_cookie_header()
    print(f"Connecting to {uri}...")
    start_time = time.time()
    frame_count = 0
    async with connect_ws(uri, headers, client.ws_ssl_context()) as websocket:
        print("Connected. Waiting for CSI frames...")
        while duration is None or time.time() - start_time < duration:
            data = await websocket.recv()
            frame_count += 1
            if not isinstance(data, bytes) or len(data) < 8:
                print(f"Invalid frame size: {len(data) if hasattr(data, '__len__') else 'n/a'}")
                continue

            offset = 0
            ts = struct.unpack_from("<I", data, offset)[0]
            offset += 4
            rssi = struct.unpack_from("<B", data, offset)[0]
            rssi = rssi - 256 if rssi > 127 else rssi
            offset += 1
            data_len = struct.unpack_from("<H", data, offset)[0]
            offset += 2
            gain = struct.unpack_from("<B", data, offset)[0] / 10.0
            offset += 1
            payload = data[offset:]
            if frame_count % 10 == 0:
                fps = frame_count / max(0.001, time.time() - start_time)
                print(
                    f"Frame #{frame_count} | FPS: {fps:.1f} | TS: {ts} | "
                    f"RSSI: {rssi}dBm | Gain: {gain:.1f} | Len: {data_len} | "
                    f"Payload: {len(payload)} bytes"
                )
    print(f"Received {frame_count} CSI frame(s)")
    return 0 if frame_count > 0 else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_common_device_args(parser)
    parser.add_argument("--duration", type=float, default=None, help="Seconds to run. Default: until Ctrl+C.")
    args = parser.parse_args(argv)
    client = DeviceClient.from_args(args)
    try:
        return asyncio.run(csi_client(client, args.duration))
    except KeyboardInterrupt:
        print("Exiting...")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
