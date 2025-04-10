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
    const double SPEED_OF_SOUND = 343.0; // m/s
    const double MIC_DISTANCE = 0.04;    // 4 cm between mics
    double angle = 0;
    int sampleRate = 32000;
    int filter_length = 5;

    Beamformer(int sampleRate, double angle) :
        sampleRate(sampleRate), 
        angle(angle) 
    {}

    // Delay-and-sum beamforming
    std::vector<int32_t> beamform(
        const std::vector<std::vector<int32_t>>& micSignals,
        double sampleRate,
        double angleDeg
    );
    std::vector<int32_t> run(
        const std::vector<std::vector<int32_t>> micSignals
    );

};