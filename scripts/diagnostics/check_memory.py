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

        print("\n--- Diagnostics endpoint removed ---")
        print("Use the /system/status page (WS snapshot) or device UI for health data.")
        return

    except Exception as e:
        print(f"Exception: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
