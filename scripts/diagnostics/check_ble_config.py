import requests
import json
import sys

# Default host
HOST = "http://192.168.0.55"

def main():
    try:
        # 1. Sign In
        print(f"Signing in to {HOST}...")
        resp = requests.post(f"{HOST}/rest/signIn", json={"username":"admin","password":"admin"}, timeout=5)
        resp.raise_for_status()
        token = resp.json().get("access_token")
        
        headers = {"Authorization": f"Bearer {token}"}

        # 2. Get BLE Config
        print("Reading BLE Config...")
        # Note: BleSettingsService uses HttpEndpoint which typically supports GET for reading
        resp = requests.get(f"{HOST}/api/ble/settings", headers=headers, timeout=5)
        
        if resp.status_code == 200:
            print(json.dumps(resp.json(), indent=2))
        else:
            print(f"Error: {resp.status_code} {resp.text}")

        # 3. Get BLE Status from API service
        print("\nReading BLE Runtime Status...")
        resp = requests.get(f"{HOST}/api/ble/status", headers=headers, timeout=5)
        if resp.status_code == 200:
             print(json.dumps(resp.json(), indent=2))

    except Exception as e:
        print(f"Exception: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
