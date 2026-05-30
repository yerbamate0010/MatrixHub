import requests
import json
import sys
import time

# Default host
HOST = "http://192.168.0.55"

def main():
    try:
        # 1. Sign In
        print(f"Signing in to {HOST}...")
        resp = requests.post(f"{HOST}/rest/signIn", json={"username":"admin","password":"admin"}, timeout=5)
        resp.raise_for_status()
        token = resp.json().get("access_token")
        if not token:
            print("ERROR: No token received")
            sys.exit(1)
        
        headers = {"Authorization": f"Bearer {token}"}

        # 2. Add Shelly Device
        print("Adding fake Shelly device...")
        payload = {
            "id": "shelly-fake-01",
            "name": "Memory Test Device",
            "ip": "192.168.0.222", # Fake IP in private range
            "enabled": True,
            "relay_index": 0
        }
        res = requests.post(f"{HOST}/api/shelly/devices", json=payload, headers=headers, timeout=5)
        
        if res.status_code == 200:
            print("Success: Shelly device added")
        else:
            print(f"Failed to add device: {res.status_code} {res.text}")
        
        # 3. Restart (Hygiene Sleep)
        print("Triggering Hygiene Restart (reboot)...")
        try:
            res = requests.post(f"{HOST}/rest/power/hygieneSleep", headers=headers, timeout=2)
        except requests.exceptions.ReadTimeout:
            print("Success: Restart triggered (timeout expected)")
        
        if res is not None and res.status_code == 200:
             print("Success: Restart command accepted")

    except Exception as e:
        print(f"Exception: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
