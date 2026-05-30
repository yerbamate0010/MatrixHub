import asyncio
import struct
import json
import requests
import websockets
import sys

# Config
HOST = "192.168.0.22"
USERNAME = "admin"
PASSWORD = "admin"
URL_LOGIN = f"http://{HOST}/rest/signIn"
URL_WS = f"ws://{HOST}/ws/csi"

async def monitor():
    print(f"Connecting to {HOST}...")
    
    # 1. Login to get Cookie
    s = requests.Session()
    try:
        resp = s.post(
            URL_LOGIN, 
            json={"username": USERNAME, "password": PASSWORD}, 
            timeout=5
        )
        resp.raise_for_status()
        print("Login successful.")
    except Exception as e:
        print(f"Login failed: {e}")
        return

    # Extract cookies for WebSocket
    cookies = s.cookies.get_dict()
    cookie_str = "; ".join([f"{k}={v}" for k, v in cookies.items()])
    
    # 2. Connect WebSocket
    headers = {"Cookie": cookie_str}
    
    print(f"Connecting WS to {URL_WS}...")
    async with websockets.connect(URL_WS, additional_headers=headers) as ws:
        print("WS Connected. Listening for CSI frames...")
        print(f"{'TS':<10} {'RSSI':<6} {'GAIN':<6} {'SCORE':<10} {'DataLen':<6}")
        print("-" * 50)
        
        try:
            import time
            start_time = time.time()
            duration = 5
            print(f"Monitoring for {duration} seconds... GO!")
            
            while (time.time() - start_time) < duration:
                try:
                    data = await asyncio.wait_for(ws.recv(), timeout=1.0)
                except asyncio.TimeoutError:
                    continue
                    
                if not isinstance(data, bytes) or len(data) < 12:
                    continue
                
                # Unpack Header (TS, RSSI, dlen, gain)
                ts, rssi_raw, dlen, gain_raw = struct.unpack_from("<IBHB", data, 0)
                
                # Conversions
                rssi = rssi_raw - 256 if rssi_raw > 127 else rssi_raw
                gain = gain_raw / 10.0
                
                print(f"{ts:<10} {rssi:<6} {gain:<6.1f} {dlen:<6}")

        except Exception as e:
            print(f"WS Error: {e}")

if __name__ == "__main__":
    try:
        asyncio.run(monitor())
    except KeyboardInterrupt:
        print("\nStopped.")
