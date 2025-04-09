#pragma once

enum sign {negative, zero, positive};

sign get_sign(float value) {
    const float threshold = 200.0f / 1000000.0f;
    sign result = zero;
    if (value < -threshold) {
        result = negative;
    } else if (value > threshold) {
        result = positive;
    }
    return result;
}

int sign_angle_finder(float tau_LF_RF, float tau_LF_LB, float tau_LF_RB,
                      float tau_RF_LB, float tau_RF_RB, float tau_LB_RB) {
    sign LF_RF = get_sign(tau_LF_RF);
    sign LF_LB = get_sign(tau_LF_LB);
    sign LF_RB = get_sign(tau_LF_RB);
    sign RF_LB = get_sign(tau_RF_LB);
    sign RF_RB = get_sign(tau_RF_RB);
    sign LB_RB = get_sign(tau_LB_RB);

    if (LF_RF == zero && LF_LB == positive && RF_RB == positive) {
        return 1;
    } else if (LF_RF == negative && LB_RB == positive) {
        return 2;
    } else if (LF_RF == negative && LB_RB == negative && RF_RB == zero) {
        return 3;
    } else if (LF_RF == negative && LB_RB == negative) {
        return 4;
    } else if (LF_RF == zero && LF_LB == negative && RF_RB == negative) {
        return 5;
    } else if (LF_RF == positive && LB_RB == negative) {
        return 6;
    } else if (LF_RF == positive && LB_RB == positive && LF_LB == zero) {
        return 7;
    } else if (LF_RF == positive && LF_LB == positive) {
        return 8;
    }
    return 0;
}