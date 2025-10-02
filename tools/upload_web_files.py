#!/usr/bin/env python3
"""
Upload web files to the hydroponic controller's flash storage
"""
import serial
import sys
import os
import time
import glob

def upload_file(ser, local_path, remote_path):
    """Upload a single file via serial"""
    with open(local_path, 'rb') as f:
        data = f.read()
    
    # Send upload command
    cmd = f"UPLOAD {remote_path} {len(data)}\n"
    ser.write(cmd.encode())
    time.sleep(0.1)
    
    # Wait for ready signal
    response = ser.readline().decode().strip()
    if response != "READY":
        print(f"Error: Expected READY, got {response}")
        return False
    
    # Send file data
    ser.write(data)
    time.sleep(0.5)
    
    # Wait for confirmation
    response = ser.readline().decode().strip()
    if response.startswith("OK"):
        print(f"✓ Uploaded {remote_path} ({len(data)} bytes)")
        return True
    else:
        print(f"✗ Failed to upload {remote_path}: {response}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: upload_web_files.py <serial_port> [web_directory]")
        print("Example: upload_web_files.py /dev/ttyACM0 web/")
        sys.exit(1)
    
    port = sys.argv[1]
    web_dir = sys.argv[2] if len(sys.argv) > 2 else "web"
    
    try:
        ser = serial.Serial(port, 115200, timeout=2)
        time.sleep(2)  # Wait for connection
        
        print(f"Connected to {port}")
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
                if upload_file(ser, local_path, remote_path):
                    success_count += 1
            else:
                print(f"⚠ Warning: {local_path} not found")
        
        print(f"\nUploaded {success_count}/{len(files_to_upload)} files")
        
        # List files on device
        ser.write(b"LIST\n")
        time.sleep(0.5)
        print("\nFiles on device:")
        while ser.in_waiting:
            print(ser.readline().decode().strip())
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

