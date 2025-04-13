import socket
import struct
import numpy as np
import time
import wave
import os
import scipy
# === Config ===
TARGET_IP = "127.0.0.1"  # Localhost for testing
TARGET_PORT = 3333
SAMPLE_RATE = 22000
N_SAMPLES = 120  # Per packet
BYTES_PER_SAMPLE = 3
GAIN = 0.8
PACKET_INTERVAL = N_SAMPLES / SAMPLE_RATE  # Pre-calculate timing

# === Optimized WAV loading ===
def load_wav_file(filename):
    if not os.path.exists(filename):
        return np.array([], dtype=np.int32)
        
    with wave.open(filename, 'rb') as wav_file:
        n_channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()
        framerate = wav_file.getframerate()
        n_frames = wav_file.getnframes()
        
        # Read and convert directly to numpy array
        frames = wav_file.readframes(n_frames)
        dtype_map = {1: np.uint8, 2: np.int16, 3: np.int32, 4: np.int32}
        samples = np.frombuffer(frames, dtype=dtype_map[sample_width])

        # Convert to mono if needed
        if n_channels == 2:
            samples = samples.reshape(-1, 2).mean(axis=1).astype(np.int32)

        # Resample using Fourier method for better quality
        if framerate != SAMPLE_RATE:
            from scipy.signal import resample
            samples = resample(samples, int(len(samples) * SAMPLE_RATE / framerate))

        # Normalize to 24-bit range
        samples = samples.astype(np.float32)
        samples *= 8388607 / np.max(np.abs(samples))
        return samples.astype(np.int32)

# === Preprocess all audio data ===
print("⚡ Preprocessing audio...")
wav_filename = "audiosnip.wav"
all_samples = load_wav_file(wav_filename)

if len(all_samples) == 0:
    print("⚠️ No WAV file found, generating test tone...")
    duration = 3
    t = np.linspace(0, duration, int(SAMPLE_RATE * duration), endpoint=False)
    all_samples = (np.sin(2 * np.pi * 440 * t) * 8388607).astype(np.int32)

# Pre-divide into chunks
total_samples = len(all_samples)
chunks = [all_samples[i:i+N_SAMPLES] for i in range(0, total_samples, N_SAMPLES)]
chunks = [c for c in chunks if len(c) == N_SAMPLES]  # Drop partial chunks
print(f"🔊 Loaded {len(chunks)} packets ({total_samples} samples)")

# === Networking Setup ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_TOS, 0x10)  # QoS priority

# === Packet Creation Templates ===
header = struct.Struct('4sBB')  # AUDI + seq + null
pre_header = b"AUDI"

# === High Precision Sending Loop ===
sequence = 0
packet_count = 0
next_send_time = time.perf_counter()

print(f"🚀 Starting streaming to {TARGET_IP}:{TARGET_PORT}")
try:
    while True:
        chunk = chunks[packet_count % len(chunks)]
        packet_count += 1

        # Vectorized 24-bit conversion (100x faster than Python loop)
        chunk_array = np.array(chunk, dtype=np.int32)
        chunk_array = np.clip(chunk_array, -2**23, 2**23-1)
        sample_bytes = chunk_array.astype('<i4').view(np.uint8).reshape(-1,4)[:,:3].tobytes()

        # Construct packet
        packet = header.pack(pre_header, sequence, 0) + sample_bytes
        
        # Send with timing precision
        sock.sendto(packet, (TARGET_IP, TARGET_PORT))
        sequence = (sequence + 1) % 256

        # Maintain precise timing using busy-wait
        next_send_time += PACKET_INTERVAL
        now = time.perf_counter()
        while now < next_send_time:
            now = time.perf_counter()

except KeyboardInterrupt:
    print("\n⏹️ Streaming stopped")
finally:
    sock.close()
