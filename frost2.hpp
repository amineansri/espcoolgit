#pragma once
#include <vector>
#include <array>
#include <cmath>


class Frost2 {
public:
    Frost2(int filterLength = 16, int numMics = 4, float mu = 0.001f);

    // The input is a 2D matrix where rows are time, and columns are microphones
    // e.g., input[time][mic] with size N x 4
    std::vector<int32_t> filter(const std::vector<std::vector<int32_t>>& input);

private:
    int J; // FIR filter length
    int K = 4; // Number of microphones
    float mu;

    std::vector<std::vector<float>> w;        // Weights [J][K]
    std::vector<float> f;                     // Constraint vector
    std::vector<std::vector<int32_t>> x_history;// Input history per mic [K][J]

    void constructConstraint();
    void applyConstraint();
    int32_t processSample(const std::array<int32_t, 4>& x_in);
    void updateWeights(const std::array<int32_t, 4>& x_in, int32_t y);
};