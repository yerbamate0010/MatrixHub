import requests
import json
import sys

# Default host
HOST = "http://192.168.0.55"

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 trigger_ble.py [enabled|disabled|scanner]")
        sys.exit(1)
        
    mode = sys.argv[1]
    
    payload = {}
    if mode in ("enabled", "scanner", "advertising", "full"):
        if mode != "enabled":
            print(f"Legacy mode '{mode}' collapsed to scanner-only BLE enable.")
        payload = {
            "enabled": True
        }
    elif mode == "disabled":
        payload = {
            "enabled": False
        }
    else:
        print(f"Unknown mode: {mode}")
        sys.exit(1)

    try:
        # 1. Sign In
        print(f"Signing in to {HOST}...")
        resp = requests.post(f"{HOST}/rest/signIn", json={"username":"admin","password":"admin"}, timeout=5)
        resp.raise_for_status()
        token = resp.json().get("access_token")
        
        headers = {"Authorization": f"Bearer {token}"}

        # 2. Configure BLE
        print(f"Configuring BLE scanner mode: {mode}...")
        print(json.dumps(payload, indent=2))
        
        # Scanner-only BLE settings expose a single persisted toggle here.
        resp = requests.post(f"{HOST}/api/ble/settings", json=payload, headers=headers, timeout=5)
        
        if resp.status_code == 200:
            print("Success: BLE settings updated")
        else:
            print(f"Failed to update settings: {resp.status_code} {resp.text}")

        # 3. Restart (Hygiene Sleep)
        print("Triggering Hygiene Restart (reboot)...")
        try:
            resp = requests.post(f"{HOST}/rest/power/hygieneSleep", headers=headers, timeout=2)
        except requests.exceptions.ReadTimeout:
            print("Success: Restart triggered (timeout expected)")
            
        if resp is not None and resp.status_code == 200:
             print("Success: Restart command accepted")

    except Exception as e:
        print(f"Exception: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
