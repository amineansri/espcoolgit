import numpy as np
import matplotlib.pyplot as plt

# === CONFIGURATION ===
array_type = 'linear'    # 'circular' or 'linear'
M = 4                    # Number of microphones
steer_angle_deg = 0      # Direction to beamform to (in degrees)
mic_spacing = 0.15      # Distance between adjacent mics (optional)
radius = None            # Used only for circular array, if mic_spacing not given

c = 343  # Speed of sound (m/s)
freqs = np.linspace(100, 1100, 500)  # Frequencies from 0 to 4 kHz
angles = np.arange(0, 360, 1)
theta = np.deg2rad(angles)

# === MIC POSITION CALCULATION ===
if array_type == 'circular':
    # Radius from mic array or distance between the mics
    if mic_spacing is not None:
        circumference = M * mic_spacing
        r = circumference / (2 * np.pi)
    elif radius is not None:
        r = radius
    mic_angles = np.linspace(0, 2 * np.pi, M, endpoint=False)
    mic_positions = np.stack((r * np.cos(mic_angles), r * np.sin(mic_angles)), axis=-1)

elif array_type == 'linear':
    # Linearly spaced along x-axis, centered at 0
    total_length = (M - 1) * mic_spacing
    x_positions = np.linspace(-total_length / 2, total_length / 2, M)
    mic_positions = np.stack((x_positions, np.zeros(M)), axis=-1)

else:
    raise ValueError("array_type must be 'circular' or 'linear'.")

# === BEAMFORMING ===
steer_angle_rad = np.deg2rad(steer_angle_deg)
steer_vec = np.array([np.cos(steer_angle_rad), np.sin(steer_angle_rad)])
steering_delays = mic_positions @ steer_vec  # dot product

response = np.zeros((len(freqs), len(theta)))

for fi, f in enumerate(freqs):
    if f == 0:
        continue
    k = 2 * np.pi * f / c
    for i, a in enumerate(theta):
        direction_vec = np.array([np.cos(a), np.sin(a)])
        delays = mic_positions @ direction_vec
        phase_shifts = np.exp(1j * k * (delays - steering_delays))
        response[fi, i] = np.abs(np.sum(phase_shifts)) / M

# === PLOTTING ===
response_db = 20 * np.log10(response / np.nanmax(response)) 
# Automatically adjust the plot limits based on the data
fig, ax = plt.subplots(figsize=(12, 6))
cax = ax.imshow(response_db, extent=[np.min(angles), np.max(angles), np.min(freqs), np.max(freqs)],
                aspect='auto', origin='lower', cmap='jet_r', vmin=-40, vmax=0)

# Add color bar and labels
fig.colorbar(cax, label='Magnitude (dB)')
ax.set_xlabel('Angle (degrees)')
ax.set_ylabel('Frequency (Hz)')
ax.set_title(f'Beamformer Response ({array_type.capitalize()} Array, Steered to {steer_angle_deg}°)')
plt.tight_layout()
plt.show()
