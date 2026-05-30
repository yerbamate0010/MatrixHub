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

        # 2. List Tasks
        print("Listing Tasks...")
        resp = requests.get(f"{HOST}/api/system/tasks", headers=headers, timeout=5)
        
        if resp.status_code == 200:
            print(json.dumps(resp.json(), indent=2))
        else:
            print(f"Error: {resp.status_code} {resp.text}")

    except Exception as e:
        print(f"Exception: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
