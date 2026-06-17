#!/usr/bin/env python3
"""Security smoke tests for HTTPS/JWT MatrixHub endpoints."""

from __future__ import annotations

import argparse
import socket
import ssl
import sys
import time
from pathlib import Path
from urllib.parse import urlsplit

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from device_client import DeviceClient, DeviceClientError, add_common_device_args  # noqa: E402


def test_content_length(client: DeviceClient) -> bool:
    print("\n--- Content-Length protection ---")
    big_data = "A" * 20000
    response = client.post(
        "/api/config",
        data=big_data,
        headers={"Content-Type": "text/plain"},
        timeout=8,
    )
    print(f"Response: HTTP {response.status_code}")
    if response.status_code == 413:
        print("[PASS] oversized payload rejected")
        return True
    lower_body = response.text.lower()
    if response.status_code == 400 and (
        "payload" in lower_body or "less than" in lower_body or "too large" in lower_body
    ):
        print("[PASS] oversized payload rejected with legacy HTTP 400")
        return True
    print(f"[FAIL] expected payload rejection, got {response.status_code}: {response.text[:200]}")
    return False


def test_slowloris(client: DeviceClient) -> bool:
    print("\n--- Slowloris protection ---")
    parts = urlsplit(client.base_url)
    host = parts.hostname
    port = parts.port or (443 if parts.scheme == "https" else 80)
    if not host:
        print("[FAIL] cannot parse device host")
        return False

    raw = socket.create_connection((host, port), timeout=5)
    raw.settimeout(5)
    conn: socket.socket | ssl.SSLSocket
    if parts.scheme == "https":
        context = ssl._create_unverified_context() if not client.verify else ssl.create_default_context()
        conn = context.wrap_socket(raw, server_hostname=host)
    else:
        conn = raw

    request = (
        "POST /api/config HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        "Content-Length: 100\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
    )
    try:
        conn.sendall(request.encode("ascii"))
        for idx in range(10):
            time.sleep(2)
            try:
                conn.sendall(b"A")
            except OSError:
                print("[PASS] server closed slow body connection")
                return True
            conn.setblocking(False)
            try:
                data = conn.recv(1024)
                if not data:
                    print("[PASS] server closed slow body connection")
                    return True
            except BlockingIOError:
                pass
            finally:
                conn.setblocking(True)
            print(f"sent slow byte {idx + 1}/10")
    except Exception as exc:
        print(f"[PASS] connection closed/failed during slow body: {exc}")
        return True
    finally:
        try:
            conn.close()
        except Exception:
            pass

    print("[FAIL] server kept slow body connection open too long")
    return False


def test_rate_limiter(client: DeviceClient) -> bool:
    print("\n--- Login rate limiter ---")
    triggered = False
    for idx in range(5):
        response = client.request(
            "POST",
            "/rest/signIn",
            auth=False,
            json={"username": "rate-limit-probe", "password": "bad"},
            timeout=3,
        )
        print(f"Attempt {idx + 1}: HTTP {response.status_code}")
        if response.status_code == 429:
            triggered = True
            break
    if triggered:
        print("[PASS] login rate limiter returned 429")
        return True
    print("[FAIL] login rate limiter did not return 429")
    return False


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_common_device_args(parser)
    parser.add_argument("--skip-slowloris", action="store_true")
    parser.add_argument("--skip-rate-limit", action="store_true")
    args = parser.parse_args(argv)

    client = DeviceClient.from_args(args)
    failures = 0
    try:
        client.login()
    except DeviceClientError as exc:
        print(f"ERROR: login failed before security tests: {exc}", file=sys.stderr)
        return 1

    if not test_content_length(client):
        failures += 1
    if not args.skip_slowloris and not test_slowloris(client):
        failures += 1
    if not args.skip_rate_limit and not test_rate_limiter(client):
        failures += 1

    if failures:
        print(f"\nFAIL: {failures} security check(s) failed")
        return 1
    print("\nPASS: security smoke checks succeeded")
    return 0


if __name__ == "__main__":
    sys.exit(main())
