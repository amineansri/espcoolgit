import numpy as np
import matplotlib.pyplot as plt
import sounddevice as sd

path = "./pythonProject.csv"
with open(path, mode="rt") as fid:
    content = fid.read()
arr = [int(x) for x in content.split(',') if x.strip()]
N_SAMPLES = len(arr)
time = np.linspace(0,1,N_SAMPLES)
print(N_SAMPLES)
# Single plot setup
# fig, ax = plt.subplots(figsize=(12, 4))
# line, = ax.plot(time, arr)
# ax.set_ylabel("Mic 1")
# ax.set_xlabel("Sample Index")
# ax.set_title(f"Real-time Audio (480 samples/packet)")
# plt.ion()
# plt.show()
#
# line.set_ydata(content)
# ax.relim()
# ax.autoscale_view()
# plt.pause(0.001)


# 2) Convert it to a NumPy array of int32
mic_samples = np.array(arr, dtype=np.int32)

# 3) Convert int32(24-bit) samples to floats in [-1.0, 1.0]
audio_float = mic_samples.astype(np.float32) / 8388607.0

# 4) Playback
sample_rate = 23000
sd.play(audio_float, samplerate=sample_rate)
sd.wait()
