import requests
import json
import time
import sys

# Default host from user request
HOST = "http://192.168.0.55"

def main():
    try:
        # 1. Sign In
        print(f"Signing in to {HOST}...")
        resp = requests.post(f"{HOST}/rest/signIn", json={"username":"admin","password":"admin"}, timeout=5)
        resp.raise_for_status()
        token = resp.json().get("access_token")
        if not token:
            print(f"ERROR: No token received in login response. Status: {resp.status_code}")
            print(f"Response: {resp.text}")
            sys.exit(1)
        
        headers = {"Authorization": f"Bearer {token}"}
        print("Success: Signed in")

        # 2. Enable Telegram
        print("Enabling Telegram mode...")
        # Only sending mode to preserve existing credentials if any
        payload = {"mode": 1}
        resp = requests.post(f"{HOST}/api/notifications/settings", json=payload, headers=headers, timeout=5)
        
        if resp.status_code == 200:
            print("Success: Notification mode disabled")
        else:
            print(f"Failed to set mode: {resp.status_code} {resp.text}")
            # Continue anyway to try restart? No, better check.
        
        # 3. Restart (Hygiene Sleep)
        print("Triggering Hygiene Restart (reboot)...")
        try:
            resp = requests.post(f"{HOST}/rest/power/hygieneSleep", headers=headers, timeout=2)
        except requests.exceptions.ReadTimeout:
            # Expected, device might reset immediately
            print("Success: Restart triggered (timeout expected)")
        
        if resp.status_code == 200:
             print("Success: Restart command accepted")

    except Exception as e:
        print(f"Exception: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
