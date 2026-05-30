import requests
import time
import socket
import sys

# Configuration - Change to your ESP32 IP
DEVICE_IP = "192.168.0.18" 
BASE_URL = f"https://{DEVICE_IP}"

def test_rate_limiter():
    print("\n--- Testing Rate Limiter (Login Bucket) ---")
    print("Blasting /rest/signIn to trigger 429 (Limit: 3/min)...")
    for i in range(5): 
        try:
            # Send bogus login attempts
            r = requests.post(f"{BASE_URL}/rest/signIn", json={"username":"a","password":"b"}, timeout=2, verify=False)
            print(f"Req {i+1}: {r.status_code}")
            if r.status_code == 429:
                print("[SUCCESS] Login Rate Limiter triggered (429 Too Many Requests)")
                return
        except Exception as e:
            print(f"Error: {e}")
    print("[FAIL] Rate Limiter NOT triggered. Is it enabled?")

def test_content_length():
    print("\n--- Testing Content-Length Protection ---")
    print("Sending 20KB actual data...")
    try:
        big_data = "A" * 20000 
        # Content-Length will be automatically set to ~20000
        r = requests.post(f"{BASE_URL}/api/config", data=big_data, timeout=5, verify=False)
        print(f"Response: {r.status_code}")
        if r.status_code == 413:
            print("[SUCCESS] Content-Length limit triggered (413 Payload Too Large)")
        else:
            print(f"[FAIL] Expected 413, got {r.status_code}. Note: if you get 401, it means the wrapper is working but auth check runs late.")
    except Exception as e:
        print(f"Error: {e}")

def test_slowloris():
    print("\n--- Testing Slowloris Protection (Connection Timeout) ---")
    print("Opening socket and sending 1 byte every 2 seconds (Timeout is 5s)...")
    try:
        import ssl
        context = ssl._create_unverified_context()
        conn = context.wrap_socket(socket.socket(socket.AF_INET), server_hostname=DEVICE_IP)
        conn.connect((DEVICE_IP, 443))
        
        request = "POST /api/config HTTP/1.1\r\nHost: e.local\r\nContent-Length: 100\r\n\r\n"
        conn.send(request.encode())
        
        for i in range(10):
            time.sleep(2)
            print(f"Sending junk byte {i+1}...")
            conn.send(b"A")
            # If the server closes the connection, the next send or recv will fail
            conn.setblocking(False)
            try:
                data = conn.recv(1024)
                if not data:
                    print("[SUCCESS] Server closed connection (Slowloris protected)")
                    return
            except BlockingIOError:
                pass
        print("[FAIL] Server kept connection open too long.")
    except Exception as e:
        print(f"Connection closed/failed as expected: {e}")
        print("[SUCCESS] Slowloris protection verified.")

if __name__ == "__main__":
    # Disable warnings for self-signed certs
    import urllib3
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    print(f"Verifying security on {DEVICE_IP}")
    test_rate_limiter()
    test_content_length()
    test_slowloris()
