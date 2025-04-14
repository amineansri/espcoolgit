#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <eigen3/Eigen/Dense>

class FrostBeamformer {
public:
    FrostBeamformer(int filterLength = 16, float mu = 0.001f, float micSpacing = 0.07f, float fs = 16000.0f);

    // The input is a 2D matrix where rows are time, and columns are microphones
    // e.g., input[time][mic] with size N x 4
    std::vector<int32_t> filter(const std::vector<std::vector<int32_t>>& input);

private:
    int J; // FIR filter length
    int K = 4; // Number of microphones
    float mu;
    float d;
    float fs;
    float c = 343.0f;

    // std::vector<float> w;        // Weights [J][K]
    // std::vector<float> f;                     // Constraint vector
    std::vector<std::vector<int32_t>> x_history;// Input history per mic [K][J]

    Eigen::MatrixXd P;
    Eigen::MatrixXd F;
    Eigen::MatrixXd f;
    Eigen::MatrixXd C;
    Eigen::VectorXd W;

    void constructConstraint();
    void applyConstraint();
    int32_t processSample(const std::array<int32_t, 4>& x_in);
    void updateWeights(const std::array<int32_t, 4>& x_in, int32_t y);
};