// Input is array 4 wide rows are the data per mice
// 4 mices linear array with 4 cm inbetween them
// Sum and delay beamformer
#include "BeamFormer.h"
#include <vector>
#include <cmath>

// Delay-and-sum beamforming
std::vector<int32_t> Beamformer::beamform(
    const std::vector<std::vector<int32_t>>& micSignals
) {
    int numMics = micSignals.size();
    int numSamples = micSignals[0].size();
    std::vector<int32_t> output(numSamples - filter_length, 0);

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

    return output;
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