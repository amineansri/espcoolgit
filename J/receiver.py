import numpy as np
import sounddevice as sd
from socket import *
from collections import deque
import threading
#This code listens to the UDP input and outputs it to the default audio device

# === Audio + Network Config ===
SAMPLE_RATE = 22000
N_SAMPLES = 120
BYTES_PER_SAMPLE = 3

UDP_IP = "0.0.0.0"
UDP_PORT = 3333

# === Setup UDP socket ===
sock = socket(AF_INET, SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"🎙️  Listening on {UDP_IP}:{UDP_PORT} — streaming to virtual mic...")
def decode_24bit_to_float(b):
    """Converts 3 bytes (little endian) to a float32 sample."""
    val = b[2] << 16 | b[1] << 8 | b[0]
    if val & 0x800000:  # Sign bit set
        val -= 0x1000000  # Convert to signed 24-bit
    return val / (2**23)
# === Audio Callback-based Stream ===
# Buffer to hold audio samples until the audio stream pulls them
audio_buffer = []

def audio_callback(outdata, frames, time, status):
    if status:
        print("Audio stream status:", status)

    # Fill with silence by default
    outdata[:] = np.zeros((frames, 1), dtype=np.float32)

    # Stream from buffer if we have enough data
    if len(audio_buffer) >= frames:
        chunk = audio_buffer[:frames]
        outdata[:, 0] = chunk
        del audio_buffer[:frames]

# === Start output stream (default output device) ===
stream = sd.OutputStream(
    samplerate=SAMPLE_RATE,
    channels=1,
    dtype='float32',
    callback=audio_callback,
    blocksize=0,
      latency='high'  # Let sounddevice decide
)
stream.start()

# === UDP receive loop ===
try:
    while True:
        data, _ = sock.recvfrom(2048)

        if len(data) < 5 or data[0:4] != b"AUDI":
            continue

        raw_data = data[5:]
        if len(raw_data) != 1 + (N_SAMPLES * BYTES_PER_SAMPLE):
            continue

        mic_raw = raw_data[1:]

        # Convert 24-bit PCM to float32
        samples = np.zeros(N_SAMPLES, dtype=np.float32)
        for i in range(N_SAMPLES):
            idx = i * 3
            s = mic_raw[idx:idx+3]
            samples[i] = decode_24bit_to_float(s)

        audio_buffer.extend(samples)

except KeyboardInterrupt:
    print("\n🛑 Stopping stream.")
finally:
    stream.stop()
    stream.close()