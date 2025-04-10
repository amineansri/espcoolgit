// Input is array 4 wide rows are the data per mice
// 4 mices linear array with 4 cm inbetween them
// Sum and delay beamformer

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Constants
const double SPEED_OF_SOUND = 343.0; // m/s
const double MIC_DISTANCE = 0.04;    // 4 cm between mics

// Delay-and-sum beamforming
std::vector<double> beamform(
    const std::vector<std::vector<double>>& micSignals,
    double sampleRate,
    double angleDeg
) {
    int numMics = micSignals[0].size();
    int numSamples = micSignals.size();
    std::vector<double> output(numSamples, 0.0);

    // Convert angle to radians
    double angleRad = angleDeg * M_PI / 180.0;

    // Calculate delays for each mic
    std::vector<int> delays(numMics);
    for (int i = 0; i < numMics; ++i) {
        double micPos = i * MIC_DISTANCE;
        double delayTime = micPos * std::sin(angleRad) / SPEED_OF_SOUND;
        delays[i] = (int)(std::round(delayTime * sampleRate));
    }

    // Apply delays and sum
    for (int t = 0; t < numSamples; ++t) {
        double sum = 0.0;
        for (int m = 0; m < numMics; ++m) {
            int idx = t - delays[m];
            if (idx >= 0 && idx < numSamples) {
                sum += micSignals[idx][m];
            }
        }
        output[t] = sum; // normalize
    }

    return output;
}

int main() {
    // Example input: micSignals[sample][mic]
    std::vector<std::vector<double>> micSignals = { };

    double sampleRate = 16000.0; // Hz
    double angle = 0.0; // degrees

    std::vector<double> beamformed = beamform(micSignals, sampleRate, angle);

    // Print output
    for (double val : beamformed) {
        std::cout << val << std::endl;
    }

    return 0;
}