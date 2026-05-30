import requests
import time
import json
import threading
import sys

# Configuration
DEVICE_IP = "192.168.0.55"
BASE_URL = f"http://{DEVICE_IP}"
USERNAME = "admin"
PASSWORD = "admin"

# Endpoints Config
# List of (method, url, safe_for_stress)
ENDPOINTS = [
    # Power
    ("GET", "/rest/power/status", True),
    # WiFi Sensing
    ("GET", "/api/wifisensing", True),
    # BLE
    ("GET", "/api/ble/status", True),
    # UDP
    ("GET", "/api/udp", True),
    # Config
    ("GET", "/api/config", True),
    # Alarms
    ("GET", "/api/alarms/rules", True),
    ("GET", "/api/alarms/status", True),
    # Heartbeat
    ("GET", "/api/heartbeat", True),
    # System
    ("GET", "/api/system/tasks", True),
    # Logs
    ("GET", "/api/logs", True),
    ("GET", "/rest/logs/tail", True), # High frequency allowed
]

def login():
    url = f"{BASE_URL}/rest/signIn"
    start = time.time()
    try:
        response = requests.post(url, json={"username": USERNAME, "password": PASSWORD}, timeout=5)
        response.raise_for_status()
        data = response.json()
        token = data.get("access_token")
        duration = (time.time() - start) * 1000
        print(f"[AUTH] Login successful in {duration:.2f}ms")
        return token
    except Exception as e:
        print(f"[AUTH] Failed: {e}")
        sys.exit(1)

def test_endpoint(endpoint, token, verbose=True):
    method, path, safe = endpoint
    url = f"{BASE_URL}{path}"
    headers = {"Authorization": f"Bearer {token}"}
    
    start = time.time()
    try:
        if method == "GET":
            response = requests.get(url, headers=headers, timeout=5)
        else:
            print(f"[SKIP] Skipping non-GET {method} {path}")
            return False, 0

        duration = (time.time() - start) * 1000
        
        if response.status_code == 200:
            if verbose:
                print(f"[PASS] {method} {path} - {response.status_code} ({duration:.2f}ms) - {len(response.content)} bytes")
            return True, duration
        else:
            print(f"[FAIL] {method} {path} - {response.status_code} ({duration:.2f}ms)")
            return False, duration
            
    except Exception as e:
        print(f"[ERR ] {method} {path} - {e}")
        return False, 0

def stress_test(token, cycles=50, delay=0.1):
    print(f"\n[STRESS] Starting stress test ({cycles} cycles, {delay}s delay)...")
    
    success_count = 0
    fail_count = 0
    total_latency = 0
    request_count = 0
    
    start_time = time.time()
    
    for i in range(cycles):
        for endpoint in ENDPOINTS:
            method, path, safe = endpoint
            if not safe: continue
            
            ok, duration = test_endpoint((method, path, safe), token, verbose=False)
            request_count += 1
            if ok:
                success_count += 1
                total_latency += duration
            else:
                fail_count += 1
            
            time.sleep(delay)
            
            if request_count % 10 == 0:
                print(f"  Progress: {request_count} requests...", end='\r')

    total_time = time.time() - start_time
    avg_latency = total_latency / success_count if success_count > 0 else 0
    
    print(f"\n[STRESS] Completed in {total_time:.2f}s")
    print(f"  Requests: {request_count}")
    print(f"  Success:  {success_count} ({success_count/request_count*100:.1f}%)")
    print(f"  Failed:   {fail_count}")
    print(f"  Avg Lat:  {avg_latency:.2f}ms")

if __name__ == "__main__":
    print(f"Target: {BASE_URL}")
    token = login()
    
    print("\n[TEST] Verifying all endpoints once...")
    for ep in ENDPOINTS:
        test_endpoint(ep, token)
        
    print("\n[PROMPT] Perform stress test? (50 cycles) [y/N]")
    # For automated running, we skip input and just do it
    stress_test(token, cycles=20, delay=0.05)
