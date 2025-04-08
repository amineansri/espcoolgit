#include "buffer.h"

#pragma once

template<typename T, int SIZE>
T cross_correlation_sub(const buffer<T, SIZE>& bufferA, const buffer<T, SIZE>& bufferB, int offset) {
    T result = T(0);

    for (int i = 0; i < SIZE; ++i) {
        result += bufferA[i] * bufferB[i + offset];
    }

    return result;
}

template<typename T, int SIZE, int sample_swing>
void cross_correlation(const buffer<T, SIZE>& bufferA, const buffer<T, SIZE>& bufferB,
                       std::array<T, sample_swing>& results) {
    for (int i = 0; i < sample_swing; ++i) {
        results[i] = cross_correlation_sub(bufferA, bufferB, i);
    }
}