#!/usr/bin/env python3
import requests
import json
import argparse
import sys

DEVICE_IP = "192.168.0.55"
BASE_URL = f"http://{DEVICE_IP}/api"

def get_token():
    try:
        auth_data = {"username": "admin", "password": "admin"}
        response = requests.post(f"http://{DEVICE_IP}/rest/signIn", json=auth_data, timeout=5)
        if response.status_code == 200:
            return response.json().get("access_token")
        print(f"Login failed: {response.status_code}")
        return None
    except Exception as e:
        print(f"Login error: {e}")
        return None

def get_config(token):
    headers = {"Authorization": f"Bearer {token}"}
    try:
        response = requests.get(f"{BASE_URL}/udp", headers=headers, timeout=5)
        if response.status_code == 200:
            print(json.dumps(response.json(), indent=2))
        else:
            print(f"Error getting config: {response.status_code} - {response.text}")
    except Exception as e:
        print(f"Connection error: {e}")

def set_config(token, enable):
    headers = {"Authorization": f"Bearer {token}"}
    # Enable with dummy host, line protocol
    payload = {
        "enabled": enable,
        "host": "192.168.0.25", # Twoj adres IP (Mac)
        "port": 8094,
        "format": "line",
        "interval_ms": 60000
    }

    try:
        print(f"Setting UDP: enabled={enable}")
        response = requests.post(f"{BASE_URL}/udp", json=payload, headers=headers, timeout=5)
        if response.status_code == 200:
            print("Success!")
            print(json.dumps(response.json(), indent=2))
        else:
            print(f"Error setting config: {response.status_code} - {response.text}")
    except Exception as e:
        print(f"Connection error: {e}")

def main():
    parser = argparse.ArgumentParser(description="UDP Config Tool")
    parser.add_argument("--enable", action="store_true", help="Enable UDP")
    parser.add_argument("--disable", action="store_true", help="Disable UDP")
    parser.add_argument("--check", action="store_true", help="Check current config")
    
    args = parser.parse_args()
    
    token = get_token()
    if not token:
        sys.exit(1)
        
    if args.enable:
        set_config(token, True)
    elif args.disable:
        set_config(token, False)

    else:
        get_config(token)

if __name__ == "__main__":
    main()
