#!/usr/bin/env python3
"""
Upload web files to the hydroponic controller via TCP
"""
import socket
import sys
import os
import time
import base64

def upload_file_tcp(host, port, local_path, remote_path):
    """Upload a single file via TCP"""
    with open(local_path, 'rb') as f:
        data = f.read()
    
    try:
        # Connect to TCP server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect((host, port))
        
        # Read welcome message
        response = sock.recv(1024).decode().strip()
        print(f"Connected: {response}")
        
        # Send upload command
        cmd = f"upload {remote_path} {len(data)}\n"
        sock.send(cmd.encode())
        
        # Wait for ready signal
        response = sock.recv(1024).decode().strip()
        if not response.startswith("READY"):
            print(f"Error: Expected READY, got {response}")
            return False
        
        print(f"Server ready, sending {len(data)} bytes...")
        
        # Encode data as base64 and send in chunks
        b64_data = base64.b64encode(data).decode()
        chunk_size = 1024  # Send in 1KB chunks
        
        for i in range(0, len(b64_data), chunk_size):
            chunk = b64_data[i:i + chunk_size]
            cmd = f"data {chunk}\n"
            sock.send(cmd.encode())
            
            # Wait for acknowledgment
            response = sock.recv(1024).decode().strip()
            if response.startswith("ERROR"):
                print(f"Error: {response}")
                return False
            elif response.startswith("OK"):
                print(f"✓ Uploaded {remote_path} ({len(data)} bytes)")
                return True
            elif response.startswith("RECEIVED"):
                print(f"Progress: {response}")
            else:
                print(f"Unexpected response: {response}")
        
        # Final check
        response = sock.recv(1024).decode().strip()
        if response.startswith("OK"):
            print(f"✓ Uploaded {remote_path} ({len(data)} bytes)")
            return True
        else:
            print(f"✗ Upload failed: {response}")
            return False
            
    except Exception as e:
        print(f"Error: {e}")
        return False
    finally:
        sock.close()

def list_files_tcp(host, port):
    """List files on the device via TCP"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((host, port))
        
        # Read welcome message
        sock.recv(1024)
        
        # Send list command
        sock.send(b"list\n")
        
        # Read response
        response = sock.recv(4096).decode().strip()
        print("Files on device:")
        print(response)
        
    except Exception as e:
        print(f"Error listing files: {e}")
    finally:
        sock.close()

def main():
    if len(sys.argv) < 2:
        print("Usage: upload_web_files_tcp.py <ip_address> [web_directory]")
        print("Example: upload_web_files_tcp.py 192.168.1.100 web/")
        sys.exit(1)
    
    host = sys.argv[1]
    port = 47293  # TCP_PORT from config
    web_dir = sys.argv[2] if len(sys.argv) > 2 else "web"
    
    print(f"Connecting to {host}:{port}")
    print(f"Uploading files from {web_dir}/\n")
    
    # Upload all files from web directory
    files_to_upload = [
        ('index.html', '/index.html'),
        ('app.css', '/app.css'),
        ('app.js', '/app.js'),
        ('favicon.ico', '/favicon.ico'),
    ]
    
    success_count = 0
    for local_name, remote_path in files_to_upload:
        local_path = os.path.join(web_dir, local_name)
        if os.path.exists(local_path):
            if upload_file_tcp(host, port, local_path, remote_path):
                success_count += 1
            time.sleep(0.5)  # Brief pause between uploads
        else:
            print(f"⚠ Warning: {local_path} not found")
    
    print(f"\nUploaded {success_count}/{len(files_to_upload)} files")
    
    # List files on device
    print("\n" + "="*50)
    list_files_tcp(host, port)

if __name__ == "__main__":
    main()
