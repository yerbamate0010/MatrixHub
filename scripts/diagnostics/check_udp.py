#!/usr/bin/env python3
"""Read or update UDP push settings over HTTPS/JWT."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from device_client import DeviceClient, DeviceClientError, add_common_device_args, print_json  # noqa: E402


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_common_device_args(parser)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--enable", action="store_true", help="Enable UDP push.")
    mode.add_argument("--disable", action="store_true", help="Disable UDP push.")
    parser.add_argument("--target-host", default="192.168.0.25")
    parser.add_argument("--target-port", type=int, default=8094)
    parser.add_argument("--format", choices=("line", "json", "csv"), default="line")
    parser.add_argument("--interval-ms", type=int, default=60000)
    args = parser.parse_args(argv)

    client = DeviceClient.from_args(args)
    try:
        if args.enable or args.disable:
            current = client.json("GET", "/api/udp")
            payload = {
                "enabled": bool(args.enable),
                "host": args.target_host if args.enable else current.get("host", args.target_host),
                "port": args.target_port if args.enable else current.get("port", args.target_port),
                "format": args.format if args.enable else current.get("format", args.format),
                "interval_ms": max(1000, args.interval_ms if args.enable else current.get("interval_ms", args.interval_ms)),
            }
            result = client.json("POST", "/api/udp", json=payload)
        else:
            result = client.json("GET", "/api/udp")
    except DeviceClientError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if not args.json:
        print(f"Target: {client.base_url}")
    print_json(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
