import asyncio
import struct
import time
import sys

# Try to import websockets library
try:
    import websockets
    import requests
    import json
except ImportError:
    print("Error: 'websockets' or 'requests' library not found.")
    print("Please run: pip3 install websockets requests")
    sys.exit(1)

import argparse

# Default config
HOST = "192.168.0.22"
URL_LOGIN = f"http://{HOST}/rest/signIn"
WS_URL_BASE = f"ws://{HOST}/ws/csi"
USERNAME = "admin"
PASSWORD = "admin"

def parse_arguments():
    parser = argparse.ArgumentParser(description='Analyze CSI data stream.')
    parser.add_argument('--duration', type=int, default=5, help='Duration of analysis in seconds')
    return parser.parse_args()

args = parse_arguments()
DURATION = args.duration  # seconds
EXPECTED_SUBCARRIERS = 64

def get_access_token():
    print(f"Logging in to {URL_LOGIN}...")
    try:
        resp = requests.post(
            URL_LOGIN, 
            json={"username": USERNAME, "password": PASSWORD}, 
            timeout=5
        )
        resp.raise_for_status()
        data = resp.json()
        token = data.get("access_token")
        if not token:
            print("Login failed: No access_token in response")
            sys.exit(1)
        print("Login successful. Token obtained.")
        return token
    except Exception as e:
        print(f"Login failed: {e}")
        sys.exit(1)

async def analyze_csi():
    # 1. Get Token
    token = get_access_token()
    
    # 2. Connect with Token
    ws_url = f"{WS_URL_BASE}?access_token={token}"
    print(f"Connecting to WS...")
    
    try:
        async with websockets.connect(ws_url) as websocket:
            print("Connected! collecting data for 5 seconds...")
            
            start_time = time.time()
            packet_count = 0
            total_bytes = 0
            
            min_amp = float('inf')
            max_amp = float('-inf')
            avg_amp_sum = 0
            avg_amp_count = 0
            
            while time.time() - start_time < DURATION:
                try:
                    message = await asyncio.wait_for(websocket.recv(), timeout=1.0)
                    total_bytes += len(message)
                    packet_count += 1
                    
                    # Parse Header (WifiSensingApiService.cpp)
                    # [0-3] Timestamp (uint32)
                    # [4] RSSI (uint8)
                    # [5-6] Data Len (uint16)
                    # [7] Gain Comp (uint8)
                    # [8-11] Motion Score (float)
                    # [12] Is Motion (uint8)
                    # [13...] Data
                    
                    HEADER_SIZE = 13
                    offset = 0
                    total_len = len(message)
                    
                    while offset < total_len:
                        if total_len - offset < HEADER_SIZE:
                            break

                        header = message[offset:offset+HEADER_SIZE]
                        # < I B H B f B (Big/Little endian? ESP32 is Little Endian "<")
                        ts, rssi, data_len, gain_scaled, motion_score, is_motion = struct.unpack("<IBHBfB", header)
                        gain_comp = gain_scaled / 10.0
                        
                        payload_start = offset + HEADER_SIZE
                        
                        if total_len - payload_start < data_len:
                            print(f"Warning: Incomplete payload. Need {data_len}, have {total_len - payload_start}")
                            break
                            
                        # Parse Amplitudes/Process Packet
                        # Just update stats for now
                        
                        offset += (HEADER_SIZE + data_len)
                        
                        # --- Stats Update ---
                        # We use the LAST packet in batch for visualization to keep it simple
                        if offset >= total_len:
                             if data_len > 0:
                                values = struct.unpack(f"{data_len}b", message[payload_start:payload_start+data_len])
                                current_min = min(values)
                                current_max = max(values)
                                current_avg = sum(map(abs, values)) / len(values)
                                
                                min_amp = min(min_amp, current_min)
                                max_amp = max(max_amp, current_max)
                                avg_amp_sum += current_avg
                                avg_amp_count += 1
                        
                except asyncio.TimeoutError:
                    continue
                except Exception as e:
                    print(f"Error receiving: {e}")
                    break
            
            duration = time.time() - start_time
            fps = packet_count / duration
            kbps = (total_bytes * 8) / 1000 / duration
            
            print("-" * 40)
            print(f"Analysis Results ({duration:.2f}s):")
            print(f"Packets Received: {packet_count}")
            print(f"Rate: {fps:.2f} FPS (Target: ~100Hz but throttled to 10 in CsiService?)") 
            # CsiService has SEND_INTERVAL_MS = 100 -> 10 FPS limit!
            print(f"Bandwidth: {kbps:.2f} kbps")
            print(f"Avg Packet Size: {total_bytes / packet_count:.1f} bytes")
            if avg_amp_count > 0:
                print(f"Raw Amplitudes (int8): Min={min_amp}, Max={max_amp}, AvgMagnitude={avg_amp_sum/avg_amp_count:.2f}")
            
            # Print last motion stats
            print(f"MVS Motion Score: {motion_score:.4f} (IsMotion: {is_motion})")
            print("-" * 40)

    except Exception as e:
        print(f"Connection failed: {e}")
        # Hint about network
        print("Make sure the ESP32 is running and reachable at 192.168.0.22")

if __name__ == "__main__":
    asyncio.run(analyze_csi())
