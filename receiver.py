import struct
from socket import *
from ctypes import c_int32
import numpy as np
import matplotlib.pyplot as plt

N_SAMPLES = 120  # Updated to 480 samples per packet
x = np.arange(N_SAMPLES)

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(("0.0.0.0", 3333))

MIC_AMOUNT = 1  # Single microphone
write_string = ""
finalarray = []
while True:
    data, addr = sock.recvfrom(2048)  # Increased buffer size for larger packets

    # Packet validation
    if len(data) < 5 or data[0:4] != b"AUDI":
        continue

    seq = data[4]  # Sequence number
    raw_data = data[5:]  # Payload starts at byte 5

    # Verify packet size (1 status byte + 480 samples * 3 bytes)
    expected_size = 1 + (120 * 3)
    if len(raw_data) != expected_size:
        print(f"Packet size mismatch! Expected {expected_size}, got {len(raw_data)}")
        continue

    # Extract samples (skip status byte at raw_data[0])
    mic_raw = raw_data[1:]  # 1440 bytes (480 samples * 3 bytes)
    #print(mic_raw)
    path = ".\pythonProject.csv"
    # Convert to 24-bit signed integers
    mic_samples = np.zeros(N_SAMPLES, dtype=np.int32)
    for i in range(N_SAMPLES):
        byte_idx = i * 3
        sample_bytes = mic_raw[byte_idx:byte_idx + 3]
        print(sample_bytes)
        # 24-bit to 32-bit conversion with sign extension
        sample = (sample_bytes[2] << 16 |
                  sample_bytes[1] << 8 |
                  sample_bytes[0])
        if sample_bytes[2] & 0x80:  # Check sign bit
            sample |= 0xFF000000  # Sign extend
        mic_samples[i] = c_int32(sample).value
        my_list = [str(number) for number in mic_samples]  # Converting ints into a string to write to the file
        write_string = ",".join(my_list)  # Join glues every individual element together using the string it got called on

    # Update plot

    with open(path, mode='at') as fid:
        fid.write(write_string + ",")

