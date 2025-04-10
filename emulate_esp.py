import socket
import struct
import numpy as np
import time
# This code emulates the ESP signal over UDP based on a CSV file \o/

# === Config ===
TARGET_IP = "127.0.0.1"  # Localhost for testing
TARGET_PORT = 3333
SAMPLE_RATE = 22000
N_SAMPLES = 120  # Per packet
BYTES_PER_SAMPLE = 3

# === Load CSV Samples ===
with open("pythonProject.csv", "r") as f:
    sample_strings = f.read().strip().split(",")
    all_samples = [int(s) for s in sample_strings if s.strip()]

print(f"Loaded {len(all_samples)} samples from CSV")

# === Socket Setup ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# === Send Loop ===
sequence = 0
i = 0
while True:
    # Get next chunk of samples
    chunk = all_samples[i:i+N_SAMPLES]
    if len(chunk) < N_SAMPLES:
        i = 0
        continue
    i += N_SAMPLES

    # Pack samples into 24-bit little-endian format
    sample_bytes = bytearray()
    for val in chunk:
        packed = struct.pack("<i", val)  # Pack full 4 bytes
        sample_bytes += packed[:3]      # Take only the first 3 bytes (little-endian)

    # Create packet
    packet = b"AUDI"                        # Header
    packet += struct.pack("B", sequence)   # Sequence byte
    packet += b"\x00"                      # Status byte (dummy)
    packet += sample_bytes                 # 120 samples × 3 bytes = 360 bytes

    # Send it
    sock.sendto(packet, (TARGET_IP, TARGET_PORT))

    sequence = (sequence + 1) % 256
    time.sleep(N_SAMPLES / SAMPLE_RATE)  # Keep real-time timing