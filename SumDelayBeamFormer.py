import numpy as np
import matplotlib.pyplot as plt

# === CONFIGURATION ===
FREQ_RESOLUTION = 500   # Number of freq points to calculate
ANGLE_RESOLUTION = 181  # Number of angle points to calculate

numElements = 8         # Number of array elements
spacing = 0.10           # Element separation in metre
speedSound = 343.0      # m/s

freqs = np.linspace(0, 3000, 500)
angles = np.arange(-90, 90, 1)
final = np.zeros((FREQ_RESOLUTION, ANGLE_RESOLUTION)) 

# Iterate through arrival angle points
for f in range(FREQ_RESOLUTION):
   freq = 3000.0 * f / (FREQ_RESOLUTION-1)

   for a in range(ANGLE_RESOLUTION):
      # Calculate the planewave arrival angle
      angle = -90 + 180.0 * a / (ANGLE_RESOLUTION-1);
      angleRad = np.pi * angle / 180;

      realSum = 0;
      imagSum = 0;

      # Iterate through array elements
      for i in range(numElements):
         # Calculate element position and wavefront delay
         position = i * spacing;
         delay = position * np.sin(angleRad) / speedSound

         # Add Wave
         realSum += np.cos(2.0 * np.pi * freq * delay)
         imagSum += np.sin(2.0 * np.pi * freq * delay)

      output = np.sqrt(realSum * realSum + imagSum * imagSum) / numElements
      logOutput = 20 * np.log10(output)
      if logOutput < -50: 
          logOutput = -50
      final[f,a] = logOutput

# === PLOTTING ===
# Automatically adjust the plot limits based on the data
fig, ax = plt.subplots(figsize=(12, 6))
cax = ax.imshow(final, extent=[np.min(angles), np.max(angles), np.min(freqs), np.max(freqs)],
                aspect='auto', origin='lower', cmap='jet_r', vmin=-40, vmax=0)

# Add color bar and labels
fig.colorbar(cax, label='Magnitude (dB)')
ax.set_xlabel('Angle (degrees)')
ax.set_ylabel('Frequency (Hz)')
# ax.set_title(f'Beamformer Response ({array_type.capitalize()} Array, Steered to {steer_angle_deg}°)')
plt.tight_layout()
plt.show()