#!/usr/bin/env python3
"""Short CSI monitor over WSS with JWT cookie auth."""

from __future__ import annotations

import argparse
import asyncio
import struct
import sys
import time
from pathlib import Path

import websockets

sys.path.insert(0, str(Path(__file__).resolve().parent))
from device_client import DeviceClient, add_common_device_args  # noqa: E402


def connect_ws(uri: str, headers: dict[str, str], ssl_context):
    try:
        return websockets.connect(uri, additional_headers=headers, ssl=ssl_context)
    except TypeError:
        return websockets.connect(uri, extra_headers=headers, ssl=ssl_context)


async def monitor(client: DeviceClient, duration: float) -> int:
    uri = client.ws_url("/ws/csi")
    count = 0
    async with connect_ws(uri, client.ws_cookie_header(), client.ws_ssl_context()) as ws:
        print(f"Connected to {uri}")
        print(f"{'TS':<10} {'RSSI':<6} {'GAIN':<6} {'LEN':<6}")
        print("-" * 34)
        start = time.time()
        while time.time() - start < duration:
            try:
                data = await asyncio.wait_for(ws.recv(), timeout=1.0)
            except asyncio.TimeoutError:
                continue
            if not isinstance(data, bytes) or len(data) < 8:
                continue
            ts, rssi_raw, dlen, gain_raw = struct.unpack_from("<IBHB", data, 0)
            rssi = rssi_raw - 256 if rssi_raw > 127 else rssi_raw
            print(f"{ts:<10} {rssi:<6} {gain_raw / 10.0:<6.1f} {dlen:<6}")
            count += 1
    return 0 if count else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_common_device_args(parser)
    parser.add_argument("--duration", type=float, default=5.0)
    args = parser.parse_args(argv)
    try:
        return asyncio.run(monitor(DeviceClient.from_args(args), max(0.1, args.duration)))
    except KeyboardInterrupt:
        print("\nStopped.")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
