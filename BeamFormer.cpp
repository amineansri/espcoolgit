// Input is array 4 wide rows are the data per mice
// 4 mices linear array with 4 cm inbetween them
// Sum and delay beamformer
#include "BeamFormer.h"
#include <iostream>
#include <iterator>
#include <vector>
#include <cmath>

// Delay-and-sum beamforming
std::vector<int32_t> Beamformer::beamform(
    const std::vector<std::vector<int32_t>>& micSignals,
    double sampleRate,
    double angleDeg
) {
    int numMics = micSignals.size();
    int numSamples = micSignals[0].size();
    std::vector<int32_t> output(numSamples - filter_length, 0);

    // Convert angle to radians
    double angleRad = angleDeg * M_PI / 180.0;

    // Calculate delays for each mic
    // std::vector<int> delays(numMics);
    // for (int i = 0; i < numMics; ++i) {
    //     double micPos = i * MIC_DISTANCE;
    //     double delayTime = micPos * std::sin(angleRad) / SPEED_OF_SOUND;
    //     delays[i] = (int)(std::round(delayTime * sampleRate));
    // }

//Sum and delay beamforming =========================================================================================
    // Apply delays and sum
    for (int t = filter_length/2; t < numSamples - filter_length/2; ++t) {
        int32_t sum = 0;
        for (int m = 0; m < numMics; ++m) {
            int idx_low = t - filter_length/2;
            int idx_hi = t + filter_length/2;
            std::vector<int32_t> aaaa(micSignals[m].begin() + idx_low,  micSignals[m].begin() + idx_hi);
            sum += interpolate(aaaa, delays[m]);
        }
        output[t - filter_length/2] = sum; // normalize
    }

//Frost beamforming ================================================================================================
    // Initialize adaptive FIR filters for each mic


    // Frost Beamforming with LMS
    // for (int t = filter_length; t < numSamples; ++t) {
    //     double y = 0;
    //     std::vector<double> x;

    //     for (int m = 0; m < numMics; ++m) {
    //         int delay = delays[m];
    //         std::vector<double> x_m(filter_length, 0);

    //         for (int k = 0; k < filter_length; ++k) {
    //             int idx = t - delay - k;
    //             if (idx >= 0) {
    //                 x_m[k] = micSignals[m][idx];
    //             }
    //         }

    //         for (int k = 0; k < filter_length; ++k) {
    //             y += weights[m][k] * x_m[k];
    //         }
    //         x.insert(x.end(), x_m.begin(), x_m.end());
    //     }

    //     output[t] = static_cast<int32_t>(y);

    //     // LMS Adaptation (update weights to minimize output power)
    //     for (int m = 0; m < numMics; ++m) {
    //         for (int k = 0; k < filter_length; ++k) {
    //             int idx = t - delays[m] - k;
    //             if (idx >= 0) {
    //                 double x_val = micSignals[m][idx];
    //                 std::cout << weights[m][k] << " ";
    //                 weights[m][k] -= filter_length * y * x_val;
    //                 std::cout << weights[m][k] << std::endl;
    //             }
    //         }
    //     }
    // }


    return output;
}

std::vector<int32_t> Beamformer::run(
        std::vector<std::vector<int32_t>> micSignals
    ) {

    std::vector<int32_t> beamformed = beamform(micSignals, sampleRate, angle);

    // Print output
    // for (int32_t val : beamformed) {
    //     std::cout << val << ", ";
    // }
    // std::cout << std::endl;
    return beamformed;
}

int32_t Beamformer::interpolate(std::vector<int32_t> signal, double delay)
{
    int centre_tap = filter_length / 2;
    double output = 0;

    for (int t=0 ; t < filter_length ; t++)
    {
        // Calculated shifted x position
        double x = t - delay;

        // Calculate sinc function value
        double sinc = sin(M_PI * (x-centre_tap)) / (M_PI * (x-centre_tap));

        // Calculate (Hamming) windowing function value
        double window = 0.54 - 0.46 * cos(2.0 * M_PI * (x+0.5) / filter_length);

        // Calculate tap weight
        double tapWeight = window * sinc;
        
        output += tapWeight * signal[t];
    }

    return (int32_t)output;
}