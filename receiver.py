import struct
from socket import *

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(("0.0.0.0", 3333))

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

    # Now process mics['1'], mics['2'], etc.