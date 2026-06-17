import argparse
import csv
import os
import struct
import time
from pathlib import Path

import requests
from urllib3.exceptions import InsecureRequestWarning

FIELDNAMES = ["timestamp", "rssi", "variance", "motion"]
DEFAULT_OUTPUT_FILE = Path(__file__).with_name("raw_capture.csv")
DEFAULT_DEVICE_URL = "https://192.168.0.30"


def parse_args():
    parser = argparse.ArgumentParser(
        description="Collect raw WiFi sensing binary snapshots and save them as CSV."
    )
    parser.add_argument(
        "--base-url",
        default=os.environ.get("DEVICE_URL", DEFAULT_DEVICE_URL),
        help="Device base URL, for example https://192.168.0.30",
    )
    parser.add_argument(
        "--username",
        default=os.environ.get("DEVICE_USERNAME", "admin"),
        help="Username used for /rest/signIn when --token is not provided.",
    )
    parser.add_argument(
        "--password",
        default=os.environ.get("DEVICE_PASSWORD", "admin"),
        help="Password used for /rest/signIn when --token is not provided.",
    )
    parser.add_argument(
        "--token",
        default=os.environ.get("DEVICE_TOKEN"),
        help="Optional bearer token. If omitted, the script logs in first.",
    )
    parser.add_argument(
        "--duration",
        type=float,
        default=120.0,
        help="Collection duration in seconds.",
    )
    parser.add_argument(
        "--poll-interval",
        type=float,
        default=2.0,
        help="Delay between requests in seconds.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_FILE,
        help="CSV output path.",
    )
    args = parser.parse_args()
    args.base_url = args.base_url.rstrip("/")
    return args


def create_session(args):
    session = requests.Session()
    session.verify = False
    requests.packages.urllib3.disable_warnings(category=InsecureRequestWarning)
    if args.token:
        session.headers.update({"Authorization": f"Bearer {args.token}"})
        return session

    response = session.post(
        f"{args.base_url}/rest/signIn",
        json={"username": args.username, "password": args.password},
        timeout=5,
    )
    response.raise_for_status()
    token = response.json().get("access_token")
    if not token:
        raise RuntimeError("signIn succeeded but no access_token was returned")
    session.headers.update({"Authorization": f"Bearer {token}"})
    return session


def iter_new_samples(payload, seen_timestamps):
    if len(payload) <= 27:
        return

    ssid_len = payload[21]
    header_size = 27 + ssid_len
    samples_data = payload[header_size:]

    for offset in range(0, len(samples_data), 10):
        chunk = samples_data[offset : offset + 10]
        if len(chunk) < 10:
            continue

        timestamp = struct.unpack_from("<I", chunk, 0)[0]
        if timestamp in seen_timestamps:
            continue

        seen_timestamps.add(timestamp)
        rssi_raw = struct.unpack_from("<B", chunk, 4)[0]
        yield {
            "timestamp": timestamp,
            "rssi": rssi_raw if rssi_raw < 128 else rssi_raw - 256,
            "variance": struct.unpack_from("<f", chunk, 5)[0],
            "motion": chunk[9],
        }


def initialize_output(output_file: Path):
    output_file.parent.mkdir(parents=True, exist_ok=True)
    with output_file.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDNAMES)
        writer.writeheader()


def append_rows(output_file: Path, rows):
    with output_file.open("a", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDNAMES)
        writer.writerows(rows)


def collect_samples(args):
    session = create_session(args)
    output_file = args.output
    url = f"{args.base_url}/api/wifisensing/binary"

    initialize_output(output_file)
    seen_timestamps = set()
    start_time = time.time()

    while (time.time() - start_time) < args.duration:
        try:
            response = session.get(url, timeout=2)
            response.raise_for_status()
            new_rows = list(iter_new_samples(response.content, seen_timestamps))
            if new_rows:
                append_rows(output_file, new_rows)
            elapsed = int(time.time() - start_time)
            print(f"[{elapsed}s] Saved {len(new_rows)} new samples.")
        except Exception as exc:
            print(f"[-] Poll error: {exc}")

        time.sleep(args.poll_interval)

    print(f"[+] Finished. Saved {len(seen_timestamps)} unique samples to {output_file}.")


if __name__ == "__main__":
    try:
        collect_samples(parse_args())
    except Exception as exc:
        raise SystemExit(f"Error: {exc}")
