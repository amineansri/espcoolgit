#include "FrostBeamformer.hpp"
#include <eigen3/Eigen/src/Core/Matrix.h>
#include <iostream>

FrostBeamformer::FrostBeamformer(int filterLength, float mu, float micSpacing, float fs)
    : J(filterLength), mu(mu), d(micSpacing), fs(fs) {
    W = Eigen::VectorXd(J*K);
    // w.resize(J*K, 1.0f);
    // f.resize(J, 1.0f);
    x_history.resize(K, std::vector<int32_t>(J, 0.0f));
    // constructConstraint();
    // applyConstraint();

    C = Eigen::MatrixXd(J * K, J);
    C.setZero();

    for (int i = 0; i < J; ++i) {
        for (int t = 0; t < J * K; ++t) {
            if (t >= i * K && t < (i + 1) * K) {
                C(t, i) = 1;
            }
        }
    }
    
    // std::cout << ((C * C.transpose()).inverse() * C).cols() << std::endl;
    // P = Eigen::MatrixXd::Identity(J, J) - C.transpose() * (C * C.transpose()).inverse() * C;
    // std::cout << (C * C.transpose()).inverse() << std::endl;

    P = Eigen::MatrixXd::Identity(J*K, J*K) - C * (C.transpose()*C).inverse()*(C.transpose());
    std::cout << P << std::endl;
    F = Eigen::MatrixXd(J, 1);
    F.setZero();
    F(0,0) = 1;
    
    // F = ((C * C.transpose()).inverse() * C) * f;

    // F = C*(C.transpose()*C).inverse()*f;

    W = Eigen::MatrixXd(J * K, 1);
    W.setZero();

    // std::cout << "Projection Matrix:\n" << F << std::endl;
    std::cout << F.rows() << std::endl;
}

// void FrostBeamformer::constructConstraint() {
//     f[0] = 1.0f; // Unit impulse constraint
// }

// void FrostBeamformer::applyConstraint() {
//     for (int j = 0; j < J; ++j)
//         for (int k = 0; k < K; ++k)
//             w[j * K + k] = f(0, j) / K;
// }

// int32_t FrostBeamformer::processSample(const std::array<int32_t, 4>& x_in) {
//     // Shift input history
//     for (int k = 0; k < K; ++k) {
//         x_history[k].insert(x_history[k].begin(), x_in[k]);
//         x_history[k].pop_back();
//     }

//     // Calculate output y[n]
//     float y = 0.0f;
//     for (int j = 0; j < J; ++j)
//         for (int k = 0; k < K; ++k)
//             y += w[j * K + k] * x_history[k][j];

//     return y;
// }

// void FrostBeamformer::updateWeights(const std::array<int32_t, 4>& x_in, int32_t y) {
//     Eigen::MatrixXd x_tilde(J*K, 1);
//     for (int j = 0; j < J; ++j)
//         for (int k = 0; k < K; ++k)
//             x_tilde(j*k) = (x_history[k][j]);

//     // std::vector<float> w_flat(K * J);
//     // for (int j = 0; j < J; ++j)
//     //     for (int k = 0; k < K; ++k)
//     //         w_flat[j * K + k] = w[j][k];

//     std::vector<float> w_tilde = w;
//     for (int j = 0; j < J; ++j)
//         for (int k = 0; k < K; ++k)
//             w[j * K + k + 1] = w_tilde[j * K + k];


//     // for (int i = 0; i < K * J; ++i)
//     //     w[i] -= mu * y * x_tilde[i];

//     // for (int j = 0; j < J; ++j)
//     //     for (int k = 0; k < K; ++k)
//     //         w[j * k] = f[j] / K;
// }

std::vector<int32_t> FrostBeamformer::filter(const std::vector<std::vector<int32_t>>& input) {
    std::vector<int32_t> output;
    output.reserve(input.size());

    int numSamples = input[0].size();
    Eigen::MatrixXd X_tap = Eigen::MatrixXd::Zero(K * J, numSamples);

    for (int k = J; k < numSamples; ++k) {
        Eigen::VectorXd xk(K * J); // temporary vector for tap-delayed stack

        for (int m = 0; m < K; ++m) {
            // Fill in J taps for microphone m, reversed (newest to oldest)
            for (int j = 0; j < J; ++j) {
                int idx = k - j; // from current (k) back to (k - J + 1)
                xk(m * J + j) = static_cast<double>(input[m][idx]);
            }
        }

        X_tap.col(k) = xk;
    }
    

    int JK = J*K;

    // Output signal
    std::vector<int32_t> Y(numSamples, 0.0);

    // Precompute constraint part: f = C * (Cᵀ * C)⁻¹ * F
    Eigen::MatrixXd CtC_inv = (C.transpose() * C).inverse();
    Eigen::VectorXd f = C * (CtC_inv * F );
    for (int k = J; k < numSamples; ++k) {
        Eigen::VectorXd xk = X_tap.col(k);               // x̃[k]
        double yk = W.dot(xk);                  // y[k] = Wᵗ * x̃[k]
        W = W - mu * P * xk * yk + f;           // weight update
        Y[k] = static_cast<int32_t>(yk);                              // store output
    }




    // for (int i = 0; i < input[0].size(); i++) {
    //     std::array<int32_t, 4> sample = {input[0][i], input[1][i], input[2][i], input[3][i]};
    //     int32_t y = processSample(sample);
    //     output.push_back(y);
    //     updateWeights(sample, y);
    // }

    return Y;
}