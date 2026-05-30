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
        if not token:
            print("ERROR: No token received")
            sys.exit(1)
        
        headers = {"Authorization": f"Bearer {token}"}

        # 2. Remove Shelly Device
        print("Removing fake Shelly device...")
        # API expects DELETE /api/shelly/devices with query param id
        resp = requests.delete(f"{HOST}/api/shelly/devices", params={"id": "shelly-fake-01"}, headers=headers, timeout=5)
        
        if resp.status_code == 200:
            print("Success: Shelly device removed")
        else:
            print(f"Failed to remove device: {resp.status_code} {resp.text}")

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
