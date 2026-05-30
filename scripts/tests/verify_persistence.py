
import requests
import time
import sys
import json

BASE_URL = "http://192.168.0.55"
AUTH_PAYLOAD = {"username": "admin", "password": "admin"}

session = requests.Session()
token = None

def login():
    global token
    print(f"[*] Logging in to {BASE_URL}/rest/signIn...")
    r = session.post(f"{BASE_URL}/rest/signIn", json=AUTH_PAYLOAD, timeout=10)
    r.raise_for_status()
    data = r.json()
    token = data.get("access_token")
    if token:
        print("[+] Login successful! Token acquired.")
        session.headers.update({"Authorization": f"Bearer {token}"})
    else:
        print("[!] Login failed: No access_token in response.")
        sys.exit(1)

def get_settings():
    print(f"[*] Fetching BLE settings...")
    r = session.get(f"{BASE_URL}/api/ble/settings", timeout=10)
    if r.status_code == 401:
        login()
        r = session.get(f"{BASE_URL}/api/ble/settings", timeout=10)
    r.raise_for_status()
    return r.json()

def normalize_settings(settings):
    sensors = []
    for sensor in settings.get("sensors", []):
        sensors.append({
            "mac": sensor.get("mac", ""),
            "alias": sensor.get("alias", "")
        })

    return {
        "enabled": bool(settings.get("enabled", False)),
        "sensors": sensors
    }

def save_settings(settings):
    print(f"[*] Saving BLE settings...")
    # BLE is scanner-only now. Persist only the live contract and avoid sending
    # removed peripheral/passkey fields back into the endpoint.
    payload = normalize_settings(settings)
    r = session.post(f"{BASE_URL}/api/ble/settings", json=payload, timeout=10)
    r.raise_for_status()
    return r.json()

def restart_device():
    print(f"[*] Requesting device restart...")
    try:
        session.post(f"{BASE_URL}/rest/restart", timeout=2)
    except requests.exceptions.RequestException:
        pass

def wait_for_online(timeout=90):
    print(f"[*] Waiting for device to come back online (up to {timeout}s)...")
    start = time.time()
    while time.time() - start < timeout:
        try:
            login()
            r = session.get(f"{BASE_URL}/api/ble/settings", timeout=2)
            if r.status_code == 200:
                print("[+] Device is back online!")
                return True
        except:
            pass
        time.sleep(5)
    return False

def main():
    try:
        # 1. Get initial state
        initial = get_settings()
        print(f"[DEBUG] Initial Settings: {json.dumps(initial, indent=2)}")
        baseline = normalize_settings(initial)

        # 2. Save the current scanner-only settings back verbatim. This exercises
        # the persistence path without reintroducing removed BLE peripheral fields.
        save_settings(baseline)
        print("[+] Scanner-only BLE settings saved back successfully")

        # 3. Restart
        restart_device()
        time.sleep(5) # Give it time to start shutdown
        
        if not wait_for_online():
            print("[!] ERROR: Device failed to come back online within timeout.")
            sys.exit(1)

        # 4. Verify
        print("[*] Verifying persistence...")
        post_restart = get_settings()
        print(f"[DEBUG] Post-Restart Settings: {json.dumps(post_restart, indent=2)}")
        persisted = normalize_settings(post_restart)

        if persisted == baseline:
            print("==================================================")
            print("   SUCCESS: BLE Scanner Settings Persisted!      ")
            print("==================================================")
        else:
            print("==================================================")
            print("   FAILED: BLE Scanner Settings Drifted!         ")
            print(f"   Expected: {json.dumps(baseline, indent=2)}")
            print(f"   Got:      {json.dumps(persisted, indent=2)}")
            print("==================================================")
            sys.exit(1)

    except Exception as e:
        print(f"[!] TEST ERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
