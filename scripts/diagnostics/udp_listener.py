#!/usr/bin/env python3
import socket
import sys

def main():
    # Define port to listen on
    port = 8094
    host = "0.0.0.0"
    
    # Initialize UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        sock.bind((host, port))
        print(f"Listening for continuous UDP packets on {host}:{port}...")
        print("Press Ctrl+C to close the listener.\n")
        
        while True:
            data, addr = sock.recvfrom(1024)
            try:
                # Try decoding as text (valid for JSON, CSV, Line Protocol)
                text = data.decode('utf-8')
                print(f"[{addr[0]}] Received: {text}", flush=True)
            except UnicodeDecodeError:
                # Fallback if raw binary data arrives
                print(f"[{addr[0]}] Received binary data: {data.hex()}", flush=True)
                
    except KeyboardInterrupt:
        print("\nListener stopped by user.")
        sys.exit(0)
    except Exception as e:
        print(f"Fatal error occurred: {e}")
        sys.exit(1)
    finally:
        sock.close()

if __name__ == "__main__":
    main()
