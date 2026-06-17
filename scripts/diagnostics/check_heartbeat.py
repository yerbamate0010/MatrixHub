#!/usr/bin/env python3
"""Read or update heartbeat settings over HTTPS/JWT."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from device_client import DeviceClient, DeviceClientError, add_common_device_args, print_json  # noqa: E402


def disabled_slots(count: int = 4) -> list[dict]:
    return [
        {"enabled": False, "name": "", "url": "", "allow_insecure": False}
        for _ in range(count)
    ]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_common_device_args(parser)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--enable", action="store_true", help="Enable a single diagnostic slot.")
    mode.add_argument("--disable", action="store_true", help="Disable all heartbeat slots.")
    parser.add_argument("--url", default="https://example.com/", help="URL for --enable.")
    parser.add_argument("--interval-ms", type=int, default=60000)
    args = parser.parse_args(argv)

    client = DeviceClient.from_args(args)
    try:
        if args.enable or args.disable:
            payload = {
                "interval_ms": max(1000, args.interval_ms),
                "slots": disabled_slots(),
            }
            if args.enable:
                payload["slots"][0] = {
                    "enabled": True,
                    "name": "Diagnostics",
                    "url": args.url,
                    "allow_insecure": False,
                }
            result = client.json("POST", "/api/heartbeat", json=payload)
        else:
            result = client.json("GET", "/api/heartbeat")
    except DeviceClientError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if not args.json:
        print(f"Target: {client.base_url}")
    print_json(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
