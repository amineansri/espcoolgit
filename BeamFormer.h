// Input is array 4 wide rows are the data per mice
// 4 mices linear array with 4 cm inbetween them
// Sum and delay beamformer

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

class Beamformer {
public:
    // Constants
    const double SPEED_OF_SOUND = 343.3; // m/s
    const double MIC_DISTANCE = 0.04;    // 4 cm between mics
    double angle = 0;
    int sampleRate = 32000;
    int filter_length = 11;
    int numMics = 4;
    const double TARGET_DISTANCE = 1.5;
    std::vector<std::vector<double>> weights;
    std::vector<double> delays;

    Beamformer(int sampleRate, double angle) :
        sampleRate(sampleRate), 
        angle(angle) 
    {
        weights = std::vector<std::vector<double>>(numMics, std::vector<double>(filter_length, 0));
        delays = std::vector<double>(numMics);
        for (int m = 0; m < numMics; ++m) {
            weights[m][filter_length / 2] = 1.0 / numMics; // Initialize to DS beamformer
            
            delays[m] = (0.25 * sampleRate * (cos(atan(TARGET_DISTANCE / ((MIC_DISTANCE/2) + MIC_DISTANCE*(m-(numMics/2))))))) / SPEED_OF_SOUND;
            std::cout << delays[m] << std::endl;
        }
    }

    // Delay-and-sum beamforming
    std::vector<int32_t> beamform(
        const std::vector<std::vector<int32_t>>& micSignals,
        double sampleRate,
        double angleDeg
    );
    std::vector<int32_t> run(
        const std::vector<std::vector<int32_t>> micSignals
    );

    int32_t interpolate(std::vector<int32_t> signal, double delay);

};