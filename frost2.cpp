#include "frost2.hpp"


Frost2::Frost2(int filterLength, int numMics, float mu)
    : J(filterLength), K(numMics),  mu(mu) {
    w.resize(J, std::vector<float>(K, 0.0f));
    f.resize(J, 0.0f);
    x_history.resize(K, std::vector<int32_t>(J, 0.0f));
    constructConstraint();
    applyConstraint();
}

std::vector<int32_t> Frost2::filter(const std::vector<std::vector<int32_t>>& input) {
    
}

