import asyncio
import websockets
import struct
import time
import sys

# Usage: python csi_client.py <IP_ADDRESS>
# Example: python csi_client.py 192.168.0.22

async def csi_client(uri):
    print(f"Connecting to {uri}...")
    try:
        async with websockets.connect(uri) as websocket:
            print("Connected! Waiting for CSI frames...")
            frame_count = 0
            start_time = time.time()
            
            while True:
                data = await websocket.recv()
                frame_count += 1
                
                # Check frame size
                # Header: 4B (Start) + 4B (TS) + 1B (RSSI) + 2B (Len) + 1B (Gain) = 12 Bytes per previous design?
                # Actually, implementation is:
                # [0-3] Timestamp (uint32)
                # [4] RSSI (uint8 -> int8)
                # [5-6] DataLen (uint16)
                # [7] Gain (uint8 -> float / 10.0)
                # [8...] Data
                
                if len(data) < 8:
                    print(f"Invalid frame size: {len(data)}")
                    continue
                    
                offset = 0
                ts = struct.unpack_from('<I', data, offset)[0]
                offset += 4
                
                rssi = struct.unpack_from('<B', data, offset)[0]
                rssi = rssi - 256 if rssi > 127 else rssi # Convert to signed
                offset += 1
                
                data_len = struct.unpack_from('<H', data, offset)[0]
                offset += 2
                
                gain_int = struct.unpack_from('<B', data, offset)[0]
                gain = gain_int / 10.0
                offset += 1
                
                payload = data[offset:]
                
                if frame_count % 10 == 0:
                    fps = frame_count / (time.time() - start_time)
                    print(f"Frame #{frame_count} | FPS: {fps:.1f} | TS: {ts} | RSSI: {rssi}dBm | Gain: {gain:.1f} | Len: {data_len} | Payload: {len(payload)} bytes")
                    
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Please provide IP address of ESP32")
        sys.exit(1)
        
    ip = sys.argv[1]
    uri = f"ws://{ip}/ws/csi"
    
    try:
        asyncio.run(csi_client(uri))
    except KeyboardInterrupt:
        print("Exiting...")
