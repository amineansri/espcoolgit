import numpy as np
import sounddevice as sd
import scipy.signal as signal
import queue
import struct
from socket import *

# === CONFIGURABLE PARAMETERS ===
fs = 16000               # Sampling rate
block_size = 1024        # Block size for processing
hop_size = block_size // 4

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(("0.0.0.0", 3333))

# === BEAMFORMER SETUP ===
M = 4                    # Number of microphones
steer_angle_deg = 0      # Direction to beamform toward (0° = front)
array_type = 'circular'  # or 'linear'
mic_spacing = 0.04       # 4 cm between microphones or used as radius
input_device = None      # Set to your mic device index (use sd.query_devices())

# === MIC GEOMETRY ===
def get_mic_positions(array_type, M, mic_spacing):
    if array_type == 'circular':
        r = (M * mic_spacing) / (2 * np.pi)
        angles = np.linspace(0, 2 * np.pi, M, endpoint=False)
        return np.stack((r * np.cos(angles), r * np.sin(angles)), axis=-1)
    elif array_type == 'linear':
        total_length = (M - 1) * mic_spacing
        x = np.linspace(-total_length / 2, total_length / 2, M)
        return np.stack((x, np.zeros(M)), axis=-1)
    else:
        raise ValueError("Unsupported array type.")

mic_positions = get_mic_positions(array_type, M, mic_spacing)
steer_rad = np.deg2rad(steer_angle_deg)
steer_dir = np.array([np.cos(steer_rad), np.sin(steer_rad)])
steering_delays = mic_positions @ steer_dir
c = 343  # Speed of sound

# === STREAMING SETUP ===
input_queue = queue.Queue()

def audio_callback(indata, frames, time, status):
    if status:
        print("Stream status:", status)
    input_queue.put(indata.copy().T)  # Transpose to shape (M, N)

def wiener_beamform_block(x_block, fs, delays):
    n_fft = block_size
    _, _, stfts = signal.stft(x_block, fs, nperseg=n_fft, noverlap=n_fft - hop_size, axis=1)
    output_stft = np.zeros_like(stfts[0], dtype=np.complex128)

    for fi, freq in enumerate(np.fft.rfftfreq(n_fft, 1/fs)):
        if freq == 0:
            continue
        k = 2 * np.pi * freq / c
        d = np.exp(-1j * k * delays)
        R = np.eye(M)  # Assumes spatially white noise
        R_inv = np.linalg.pinv(R)
        w = R_inv @ d / (d.conj().T @ R_inv @ d)

        for ti in range(stfts.shape[2]):
            x_f = stfts[:, fi, ti]
            output_stft[fi, ti] = np.vdot(w, x_f)

    _, y_block = signal.istft(output_stft, fs, nperseg=n_fft, noverlap=n_fft - hop_size)
    return y_block.real

# === MAIN STREAMING LOOP ===
print("Querying audio devices...")
print(sd.query_devices())  # To help you find your input device index

print("\nStarting real-time beamforming...")
try:
    while True:
        data, addr = sock.recvfrom(65535)
    
        # Parse header
        header = data[0:4]
        if header != b"AUDI":
            continue  # Invalid packet
    
        seq = data[4]  # Sequence number
        ptr = 5  # Pointer to start of mic data
    
        mics = {}
        while ptr < len(data):
            # Read identifier and length
            mic_id = chr(data[ptr])
            ptr += 1
            (mic_len,) = struct.unpack(">H", data[ptr:ptr + 2])
            ptr += 2
    
            # Extract audio data (24-bit samples)
            mic_data = data[ptr:ptr + mic_len]
            ptr += mic_len
    
            # Store in dictionary
            mics[mic_id] = mic_data
        output = wiener_beamform_block(mics, fs, steering_delays)
except KeyboardInterrupt:
    print("\nBeamforming stopped.")