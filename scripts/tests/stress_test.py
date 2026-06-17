#!/usr/bin/env python3
"""HTTPS/JWT endpoint stress smoke for MatrixHub."""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from device_client import DeviceClient, DeviceClientError, add_common_device_args  # noqa: E402


ENDPOINTS = [
    ("GET", "/api/system/info"),
    ("GET", "/api/system/tasks?details=1"),
    ("GET", "/api/system/network"),
    ("GET", "/api/diagnostics/summary"),
    ("GET", "/api/diagnostics/heap"),
    ("GET", "/api/diagnostics/tasks?details=0"),
    ("GET", "/api/diagnostics/mutexes"),
    ("GET", "/api/diagnostics/endpoints"),
    ("GET", "/api/diagnostics/features"),
    ("GET", "/api/config"),
    ("GET", "/rest/power/status"),
    ("GET", "/api/ble/status"),
    ("GET", "/api/ble/settings"),
    ("GET", "/api/alarms/rules?includeStatus=1"),
    ("GET", "/api/matrix/settings"),
    ("GET", "/api/wifisensing/config"),
    ("GET", "/api/heartbeat"),
    ("GET", "/api/udp"),
    ("GET", "/api/notifications/settings"),
    ("GET", "/api/macros/status"),
    ("GET", "/api/logs"),
    ("GET", "/rest/logs/tail"),
]


def hit(client: DeviceClient, method: str, path: str) -> dict:
    start = time.perf_counter()
    response = client.request(method, path)
    elapsed_ms = (time.perf_counter() - start) * 1000
    return {
        "method": method,
        "path": path,
        "status": response.status_code,
        "ok": response.status_code == 200,
        "latency_ms": round(elapsed_ms, 2),
        "bytes": len(response.content),
    }


def run(client: DeviceClient, cycles: int, delay: float, verbose: bool) -> tuple[list[dict], int]:
    results: list[dict] = []
    failures = 0
    total = cycles * len(ENDPOINTS)
    request_no = 0
    for _ in range(cycles):
        for method, path in ENDPOINTS:
            request_no += 1
            try:
                result = hit(client, method, path)
            except Exception as exc:
                result = {
                    "method": method,
                    "path": path,
                    "status": None,
                    "ok": False,
                    "latency_ms": 0,
                    "bytes": 0,
                    "error": str(exc),
                }
            results.append(result)
            failures += 0 if result["ok"] else 1
            if verbose:
                label = "PASS" if result["ok"] else "FAIL"
                print(
                    f"[{label}] {method} {path} status={result['status']} "
                    f"{result['latency_ms']:.2f}ms bytes={result['bytes']}"
                )
            elif request_no % 10 == 0:
                print(f"  progress: {request_no}/{total} requests", end="\r")
            if delay > 0:
                time.sleep(delay)
    if not verbose:
        print()
    return results, failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_common_device_args(parser)
    parser.add_argument("--cycles", type=int, default=20, help="Stress cycles. Default: 20.")
    parser.add_argument("--delay", type=float, default=0.05, help="Delay between requests. Default: 0.05.")
    parser.add_argument("--quiet", action="store_true", help="Only print summary.")
    args = parser.parse_args(argv)

    client = DeviceClient.from_args(args)
    try:
        client.login()
    except DeviceClientError as exc:
        print(f"[AUTH] {exc}", file=sys.stderr)
        return 1

    print(f"Target: {client.base_url}")
    results, failures = run(client, max(1, args.cycles), max(0.0, args.delay), not args.quiet)
    successes = len(results) - failures
    avg_latency = (
        sum(r["latency_ms"] for r in results if r["ok"]) / successes if successes else 0.0
    )
    summary = {
        "target": client.base_url,
        "requests": len(results),
        "success": successes,
        "failed": failures,
        "avg_latency_ms": round(avg_latency, 2),
    }
    if args.json:
        print(json.dumps({"summary": summary, "results": results}, indent=2, sort_keys=True))
    else:
        print(
            f"[STRESS] requests={summary['requests']} success={successes} "
            f"failed={failures} avg_latency={avg_latency:.2f}ms"
        )
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
