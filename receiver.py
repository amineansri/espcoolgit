import struct
from socket import *
from ctypes import c_int32

import numpy as np
import matplotlib.pyplot as plt

N_SAMPLES = 120  # number of samples per mic (from each UDP packet)
x = np.arange(N_SAMPLES)

fig, axs = plt.subplots(4, 1, figsize=(10, 6), sharex=True)
lines = [ax.plot(x, np.zeros(N_SAMPLES))[0] for ax in axs]
[ax.set_ylabel(f"Mic {i+1}") for i, ax in enumerate(axs)]
axs[-1].set_xlabel("Sample index")
plt.ion()
plt.show()

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(("0.0.0.0", 3333))

MIC_AMOUNT = 4

while True:
    data, addr = sock.recvfrom(2048)

    # Parse header
    header = data[0:4]
    if header != b"AUDI":
        continue  # Invalid packet

    seq = data[4]  # Sequence number


    raw_data = data[5:]
    raw_length = (len(raw_data) - MIC_AMOUNT) // MIC_AMOUNT

    raw_mic_data = []
    offset = 0
    for i in range(MIC_AMOUNT):
        offset += 1
        raw_mic_data.append(raw_data[offset : offset + raw_length])
        print(offset)
        offset += raw_length

    mic_data = []
    for j in range(MIC_AMOUNT):
        mic_data.append([])
        rmd = raw_mic_data[j]
        for i in range(0, len(rmd), 3):
            mic_data[j].append(c_int32((rmd[i+2] << 16 | rmd[i+1] << 8 | rmd[i]) | (0xFF000000 if rmd[i+2] >> 7 == 0x01 else 0x00000000)).value)

    # print(mic_data)
        # Update plots
    for i in range(MIC_AMOUNT):
        lines[i].set_ydata(mic_data[i])
        axs[i].relim()
        axs[i].autoscale_view()

    plt.pause(0.001)  # Allow the plot to update