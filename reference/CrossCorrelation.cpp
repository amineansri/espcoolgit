#pragma once
#include "cross_correlation.h"
#include <utility>

template<typename T, int SIZE>
int get_maximum(T (&array)[SIZE]){
    T maximum = 0;
    int index = 0;
    for(int i=1; i < SIZE; i++){
        if(array[i] > maximum){
            maximum = array[i];
            index = i;
        }
    }
    return index;
}

template<typename T, int SIZE>
int angle_handler(){
    float tau_LF_RF = 0;
    float tau_LF_LB = 0;
    float tau_LF_RB = 0;
    float tau_RF_LB = 0;
    float tau_RF_RB = 0;
    float tau_LB_RB = 0;

    float delayfix = 775.0f/1000000.0f;
    int offset = delayfix * Fs_mic;

    cross_correlation<float, SIZE, sample_swing>(bufferLF, bufferRF, cross_correlation_buffer, 0);
    tau_LF_RF = float(get_maximum(cross_correlation_buffer) - sample_swing) / Fs_mic + delayfix;

    cross_correlation<float, SIZE, sample_swing>(bufferLF, bufferLB, cross_correlation_buffer, 0);
    tau_LF_LB = float(get_maximum(cross_correlation_buffer) - sample_swing) / Fs_mic;

    cross_correlation<float, SIZE, sample_swing>(bufferLF, bufferRB, cross_correlation_buffer, 0);
    tau_LF_RB = float(get_maximum(cross_correlation_buffer) - sample_swing) / Fs_mic;

    cross_correlation<float, SIZE, sample_swing>(bufferRF, bufferLB, cross_correlation_buffer, 0);
    tau_RF_LB = float(get_maximum(cross_correlation_buffer) - sample_swing) / Fs_mic;

    cross_correlation<float, SIZE, sample_swing>(bufferRF, bufferRB, cross_correlation_buffer, 0);
    tau_RF_RB = float(get_maximum(cross_correlation_buffer) - sample_swing) / Fs_mic;

    cross_correlation<float, SIZE, sample_swing>(bufferLB, bufferRB, cross_correlation_buffer, 0);
    tau_LB_RB = float(get_maximum(cross_correlation_buffer) - sample_swing) / Fs_mic - delayfix;

    // Debug prints (commented)
    // Serial.print("tau_LF_RF:\t");
    // Serial.print(tau_LF_RF*1000000);
    // Serial.print("\ttau_LB_RB:\t");
    // Serial.println(tau_LB_RB*1000000);

    // Serial.print("\ttau_LF_LB:\t");
    // Serial.print(tau_LF_LB*1000000);
    // Serial.print("\ttau_RF_RB:\t");
    // Serial.println(tau_RF_RB*1000000);

    // Serial.print("\ttau_LF_RB:\t");
    // Serial.print(tau_LF_RB*1000000);
    // Serial.print("\ttau_RF_LB:\t");
    // Serial.println(tau_RF_LB*1000000);

    // Region calculation using sign angle finder
    // int region = my_least_mean_square.find_region(tau_LF_RF, tau_LF_LB, tau_LF_RB, tau_RF_LB, tau_RF_RB, tau_LB_RB);

    int region = sign_angle_finder(tau_LF_RF, tau_LF_LB, tau_LF_RB, tau_RF_LB, tau_RF_RB, tau_LB_RB);

    switch(region){
        case 0:
            break;
        case 1:
            Serial.println("front");
            break;
        case 2:
            Serial.println("front right");
            break;
        case 3:
            Serial.println("right");
            break;
        case 4:
            Serial.println("back right");
            break;
        case 5:
            Serial.println("back");
            break;
        case 6:
            Serial.println("back left");
            break;
        case 7:
            Serial.println("left");
            break;
        case 8:
            Serial.println("front left");
            break;
        default:
            break;
    }

    // Serial.println(region);
    return region;
}
