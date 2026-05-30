
import requests
import time
import sys
import json

BASE_URL = "http://192.168.0.22"
AUTH_PAYLOAD = {"username": "admin", "password": "admin"}

session = requests.Session()

def login():
    print(f"[*] Logging in to {BASE_URL}/rest/signIn...")
    try:
        r = session.post(f"{BASE_URL}/rest/signIn", json=AUTH_PAYLOAD, timeout=5)
        r.raise_for_status()
        token = r.json().get("access_token")
        if token:
            session.headers.update({"Authorization": f"Bearer {token}"})
            print("[+] Login successful.")
        else:
            print("[!] Login failed: No token.")
            sys.exit(1)
    except Exception as e:
        print(f"[!] Login error: {e}")
        sys.exit(1)

def toggle_feature(name, api_endpoint, key, current_state):
    print(f"\n[*] Toggling {name} {'OFF' if current_state else 'ON'}...")
    try:
        # Fetch current config first to preserve other fields
        r = session.get(f"{BASE_URL}{api_endpoint}", timeout=5)
        r.raise_for_status()
        config = r.json()
        
        # Toggle
        new_state = not current_state
        config[key] = new_state
        
        # Send update
        r = session.post(f"{BASE_URL}{api_endpoint}", json=config, timeout=5)
        r.raise_for_status()
        
        print(f"[+] {name} set to {new_state}. Check Serial Monitor for confirmation logs.")
        return new_state
    except Exception as e:
        print(f"[!] Failed to toggle {name}: {e}")
        return current_state

def main():
    login()
    
    # 1. BLE Toggle
    # Assumes /api/ble/settings and key "enabled"
    # First get current state
    try:
        r = session.get(f"{BASE_URL}/api/ble/settings")
        ble_enabled = r.json().get("enabled", True)
        print(f"[*] Current BLE State: {ble_enabled}")
        
        # Toggle OFF then ON
        ble_enabled = toggle_feature("BLE", "/api/ble/settings", "enabled", ble_enabled)
        time.sleep(2)
        ble_enabled = toggle_feature("BLE", "/api/ble/settings", "enabled", ble_enabled)
        
    except Exception as e:
        print(f"[!] BLE Test Error: {e}")

    # 2. WiFi Sensing Toggle
    try:
        endpoint = "/api/wifisensing/config"
        r = session.get(f"{BASE_URL}{endpoint}")
        if r.status_code == 200:
            ws_enabled = r.json().get("enabled", True)
            print(f"[*] Current WiFi Sensing State: {ws_enabled}")
            
            ws_enabled = toggle_feature("WiFi Sensing", endpoint, "enabled", ws_enabled)
            time.sleep(2)
            ws_enabled = toggle_feature("WiFi Sensing", endpoint, "enabled", ws_enabled)
        else:
            print(f"[!] WiFi Sensing endpoint not found (Status {r.status_code})")

    except Exception as e:
        print(f"[!] WiFi Sensing Test Error: {e}")

    # 3. Telegram (Notifications)
    try:
        endpoint = "/api/notifications/settings"
        r = session.get(f"{BASE_URL}{endpoint}")
        if r.status_code == 200:
            notif_config = r.json()
            # Telegram usually under "telegram" object or similar. 
            # If flat structure: "telegram_enabled". If nested: config["telegram"]["enabled"]
            # Let's inspect response
            print(f"[*] Notification Config Keys: {notif_config.keys()}")
            
            # Simple toggle of global notification enabled if exists, or telegram specific
            target_key = "telegram_enabled" if "telegram_enabled" in notif_config else "enabled"
            
            val = notif_config.get(target_key, False)
            print(f"[*] Current Notification/Telegram State: {val}")
            
            toggle_feature("Notifications", endpoint, target_key, val)
            time.sleep(2)
            toggle_feature("Notifications", endpoint, target_key, not val)
            
        else:
            print(f"[!] Notifications endpoint not found (Status {r.status_code})")
            
    except Exception as e:
        print(f"[!] Notifications Test Error: {e}")

if __name__ == "__main__":
    main()
