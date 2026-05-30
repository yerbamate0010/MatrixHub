#!/usr/bin/env python3
"""Long-running RAM / task watermark observer for the MatrixHub firmware.

Polls `/api/system/info` and `/api/system/tasks` at a fixed interval and
appends a CSV row per sample. Designed to be left running for hours or days
to catch slow memory leaks, fragmentation drift, and stack-watermark erosion
that don't show up in `stress_test.py`'s burst-style hammering.

Typical run before a release:

    python scripts/tests/soak_test.py --duration 24h --plot

Quick local sanity check:

    python scripts/tests/soak_test.py --duration 5m --interval 10s --plot

CSV columns:
    timestamp_iso, uptime_sec, free_heap, min_free_heap, used_heap,
    free_psram, used_psram, core_temp_c, task_count,
    min_stack_watermark, min_stack_task_name
"""

from __future__ import annotations

import argparse
import csv
import datetime
import os
import re
import signal
import statistics
import sys
import time
from pathlib import Path
from typing import Iterable

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

DEFAULT_HOST = os.environ.get("DEVICE_URL", "http://192.168.0.55")
DEFAULT_USER = os.environ.get("DEVICE_USER", "admin")
DEFAULT_PASSWORD = os.environ.get("DEVICE_PASSWORD", "admin")

CSV_FIELDS = [
    "timestamp_iso",
    "uptime_sec",
    "free_heap",
    "min_free_heap",
    "used_heap",
    "free_psram",
    "used_psram",
    "core_temp_c",
    "task_count",
    "min_stack_watermark",
    "min_stack_task_name",
]

DURATION_RE = re.compile(r"^\s*(\d+)\s*([smhd]?)\s*$", re.IGNORECASE)


def parse_duration(text: str) -> float:
    """Accept "30s", "5m", "24h", "1d" or a raw integer (seconds). "0" = infinite."""
    if not text:
        raise argparse.ArgumentTypeError("empty duration")
    match = DURATION_RE.match(text)
    if not match:
        raise argparse.ArgumentTypeError(f"unrecognized duration: {text!r}")
    value = int(match.group(1))
    unit = match.group(2).lower() or "s"
    multiplier = {"s": 1, "m": 60, "h": 3600, "d": 86400}[unit]
    return float(value * multiplier)


def parse_interval(text: str) -> float:
    seconds = parse_duration(text)
    if seconds < 1:
        raise argparse.ArgumentTypeError("interval must be >= 1 second")
    return seconds


def build_session() -> requests.Session:
    session = requests.Session()
    retry = Retry(
        total=3,
        backoff_factor=0.5,
        status_forcelist=(500, 502, 503, 504),
        allowed_methods=frozenset({"GET", "POST"}),
    )
    adapter = HTTPAdapter(max_retries=retry)
    session.mount("http://", adapter)
    session.mount("https://", adapter)
    session.verify = False
    return session


def login(session: requests.Session, host: str, user: str, password: str) -> str:
    response = session.post(
        f"{host}/rest/signIn",
        json={"username": user, "password": password},
        timeout=10,
    )
    response.raise_for_status()
    token = response.json().get("access_token")
    if not token:
        raise RuntimeError("signIn succeeded but no access_token in response")
    return token


def get_with_auth(
    session: requests.Session,
    host: str,
    path: str,
    token_holder: dict,
    creds: tuple[str, str],
) -> dict | None:
    headers = {"Authorization": f"Bearer {token_holder['token']}"}
    try:
        response = session.get(f"{host}{path}", headers=headers, timeout=10)
    except requests.RequestException as exc:
        print(f"  warning: GET {path} failed: {exc}", file=sys.stderr)
        return None

    if response.status_code == 401:
        try:
            token_holder["token"] = login(session, host, *creds)
            headers["Authorization"] = f"Bearer {token_holder['token']}"
            response = session.get(f"{host}{path}", headers=headers, timeout=10)
        except Exception as exc:
            print(f"  warning: re-auth failed: {exc}", file=sys.stderr)
            return None

    if not response.ok:
        print(
            f"  warning: GET {path} returned {response.status_code}",
            file=sys.stderr,
        )
        return None
    try:
        return response.json()
    except ValueError:
        print(f"  warning: GET {path} returned non-JSON body", file=sys.stderr)
        return None


def extract_sample(info: dict | None, tasks: dict | None) -> dict:
    info = info or {}
    tasks = tasks or {}
    task_list = tasks.get("tasks") or []

    min_watermark = None
    min_watermark_name = ""
    for task in task_list:
        wm = task.get("stackHighWaterMark")
        if wm is None:
            continue
        if min_watermark is None or wm < min_watermark:
            min_watermark = wm
            min_watermark_name = task.get("name", "")

    return {
        "timestamp_iso": datetime.datetime.now(datetime.timezone.utc)
        .isoformat(timespec="seconds"),
        "uptime_sec": info.get("uptime"),
        "free_heap": info.get("free_heap"),
        "min_free_heap": info.get("min_free_heap"),
        "used_heap": info.get("used_heap"),
        "free_psram": info.get("free_psram"),
        "used_psram": info.get("used_psram"),
        "core_temp_c": info.get("core_temp"),
        "task_count": tasks.get("taskCount"),
        "min_stack_watermark": min_watermark,
        "min_stack_task_name": min_watermark_name,
    }


def summarize(samples: list[dict], threshold: float) -> int:
    if not samples:
        print("no samples collected")
        return 1

    def column(key: str) -> list[int]:
        return [s[key] for s in samples if s.get(key) is not None]

    free = column("free_heap")
    min_free = column("min_free_heap")
    psram = column("free_psram")
    watermark = column("min_stack_watermark")

    print("\n=== soak summary ===")
    print(f"samples              : {len(samples)}")
    if free:
        print(
            f"free_heap     min/med/max: {min(free)} / "
            f"{int(statistics.median(free))} / {max(free)}"
        )
    if min_free:
        first, last = min_free[0], min_free[-1]
        drift = first - last
        drift_pct = drift / first if first else 0.0
        print(
            f"min_free_heap first/last  : {first} -> {last} "
            f"(drift {drift:+d} = {drift_pct:+.1%})"
        )
    if psram:
        print(
            f"free_psram    min/med/max: {min(psram)} / "
            f"{int(statistics.median(psram))} / {max(psram)}"
        )
    if watermark:
        worst = min(watermark)
        worst_idx = watermark.index(worst)
        worst_task = [
            s["min_stack_task_name"]
            for s in samples
            if s.get("min_stack_watermark") is not None
        ][worst_idx]
        print(f"worst stack watermark    : {worst} bytes (task: {worst_task})")

    if min_free and len(min_free) >= 2:
        first, last = min_free[0], min_free[-1]
        if first > 0 and (first - last) / first > threshold:
            print(
                f"\nFAIL: min_free_heap dropped more than "
                f"{threshold:.0%} (configured --regression-threshold)"
            )
            return 1
    print("\nPASS")
    return 0


def maybe_plot(samples: list[dict], output: Path) -> None:
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt  # noqa: WPS433
    except ImportError:
        print("matplotlib not installed; skipping --plot", file=sys.stderr)
        return

    times = list(range(len(samples)))
    fig, axes = plt.subplots(4, 1, figsize=(10, 12), sharex=True)
    series: Iterable[tuple] = (
        ("free_heap", "free heap (bytes)"),
        ("min_free_heap", "min free heap (bytes)"),
        ("free_psram", "free psram (bytes)"),
        ("min_stack_watermark", "worst stack watermark (bytes)"),
    )
    for ax, (key, label) in zip(axes, series):
        values = [s.get(key) for s in samples]
        ax.plot(times, values, marker=".", linewidth=1)
        ax.set_ylabel(label)
        ax.grid(True, alpha=0.3)
    axes[-1].set_xlabel("sample index")
    fig.suptitle(f"soak test: {output.name}")
    plot_path = output.with_suffix(".png")
    fig.tight_layout()
    fig.savefig(plot_path, dpi=120)
    print(f"plot saved to {plot_path}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--host", default=DEFAULT_HOST, help="Base URL incl. scheme.")
    parser.add_argument("--user", default=DEFAULT_USER)
    parser.add_argument("--password", default=DEFAULT_PASSWORD)
    parser.add_argument(
        "--interval",
        type=parse_interval,
        default=30.0,
        help="Seconds between samples (e.g. 30, 30s, 1m). Default 30s.",
    )
    parser.add_argument(
        "--duration",
        type=parse_duration,
        default=parse_duration("24h"),
        help="Total run time (e.g. 24h, 30m, 0=infinite). Default 24h.",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="CSV output path. Default: soak_<UTC_timestamp>.csv",
    )
    parser.add_argument(
        "--regression-threshold",
        type=float,
        default=0.10,
        help="Fail if (first_min_free - last_min_free) / first_min_free "
        "exceeds this fraction. Default 0.10 (10%%).",
    )
    parser.add_argument(
        "--plot",
        action="store_true",
        help="Generate <output>.png chart at the end (requires matplotlib).",
    )
    args = parser.parse_args(argv)

    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    output_path = Path(args.output) if args.output else Path(f"soak_{stamp}.csv")

    session = build_session()
    try:
        token = login(session, args.host, args.user, args.password)
    except Exception as exc:
        sys.exit(f"login failed: {exc}")

    token_holder = {"token": token}
    creds = (args.user, args.password)

    interrupted = {"stop": False}

    def _handle_sigint(*_):
        interrupted["stop"] = True
        print("\n[interrupt received — finishing current sample]", file=sys.stderr)

    signal.signal(signal.SIGINT, _handle_sigint)

    samples: list[dict] = []
    deadline = time.monotonic() + args.duration if args.duration > 0 else None

    print(
        f"polling {args.host} every {args.interval:.0f}s, "
        f"duration={'infinite' if deadline is None else f'{args.duration:.0f}s'}, "
        f"output={output_path}"
    )

    with output_path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=CSV_FIELDS)
        writer.writeheader()

        while not interrupted["stop"]:
            sample_start = time.monotonic()
            info = get_with_auth(
                session, args.host, "/api/system/info", token_holder, creds
            )
            tasks = get_with_auth(
                session, args.host, "/api/system/tasks", token_holder, creds
            )
            sample = extract_sample(info, tasks)
            samples.append(sample)
            writer.writerow(sample)
            fh.flush()

            print(
                f"  {sample['timestamp_iso']} "
                f"free={sample['free_heap']} "
                f"min_free={sample['min_free_heap']} "
                f"psram={sample['free_psram']} "
                f"wm={sample['min_stack_watermark']} ({sample['min_stack_task_name']})"
            )

            if deadline is not None and time.monotonic() >= deadline:
                break

            elapsed = time.monotonic() - sample_start
            sleep_for = max(0.0, args.interval - elapsed)
            # Sleep in 1 s slices so SIGINT cuts in quickly.
            slept = 0.0
            while slept < sleep_for and not interrupted["stop"]:
                step = min(1.0, sleep_for - slept)
                time.sleep(step)
                slept += step

    if args.plot:
        maybe_plot(samples, output_path)

    return summarize(samples, args.regression_threshold)


if __name__ == "__main__":
    sys.exit(main())
