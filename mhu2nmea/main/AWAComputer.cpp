#include "AWAComputer.h"

float AWAComputer::computeAwa(int16_t r, int16_t g, int16_t b, float dt_sec) {
    float a = float(r + g + b) / 3.f;

    float r_u = scale_adc(r, a);
    float g_u = scale_adc(g, a);
    float b_u = scale_adc(b, a);

    float angle120;
    // Divide into 120 degrees zone where each projection is monotonic
    if( g_u < 0 && b_u >= 0 )  // [30; 150] degrees
        angle120 = angle_red_0_180(r_u);
    else if( b_u < 0 && r_u >= 0 )  // [150; 270] degrees
        angle120 = angle_green_120_300(g_u);
    else  // [270; 30] degrees
        angle120 = angle_blue_240_060(b_u);

    // Use this angle estimate to divide into 60 degrees zones where the arc sine is most linear with respect to the angle
    // and far away form the top and bottom of the arc sine function

    float angle_red;
    float angle_green;
    float angle_blue;
    if(angle120 < M_PI / 3.f ) {  // [0; 60] degrees: green is most linear here
        angle_red = angle_red_0_180(r_u);
        angle_green = angle_green_300_120(g_u);
        angle_blue = angle_blue_240_060(b_u);
    }else if(angle120 < 2.f * M_PI / 3.f ) {  // [60; 120] degrees: red is most linear here
        angle_red = angle_red_0_180(r_u);
        angle_green = angle_green_300_120(g_u);
        angle_blue = angle_blue_060_240(b_u);
    }else if(angle120 < M_PI ) { // [120; 180] degrees: blue is most linear here
        angle_red = angle_red_0_180(r_u);
        angle_green = angle_green_120_300(g_u);
        angle_blue = angle_blue_060_240(b_u);
    }else if(angle120 < 4.f * M_PI / 3.f ) { // [180; 240] degrees: green is most linear here
        angle_red = angle_red_180_360(r_u);
        angle_green = angle_green_120_300(g_u);
        angle_blue = angle_blue_060_240(b_u);
    }else if(angle120 < 5.f * M_PI / 3.f ) { // [240; 300] degrees: red is most linear here
        angle_red = angle_red_180_360(r_u);
        angle_green = angle_green_120_300(g_u);
        angle_blue = angle_blue_240_060(b_u);
    }else { // [300; 360] degrees: blue is most linear here
        angle_red = angle_red_180_360(r_u);
        angle_green = angle_green_300_120(g_u);
        angle_blue = angle_blue_240_060(b_u);
    }

    float red_w = get_weight(r_u);
    float green_w = get_weight(g_u);
    float blue_w = get_weight(b_u);

    float angle = get_angle_w(angle_blue, angle_green, angle_red, blue_w, green_w, red_w);

    float filtered_angle = m_awaFilter.filterAngle(angle, dt_sec);

    return filtered_angle;
}

float AWAComputer::scale_adc(int16_t adc, float ampl) {
    float s = float(adc) / ampl - 1.f;

    // Most of the time sin() should be in [-1; 1]
    if( s > 1.)
        return 1.;
    else if( s < -1.)
        return -1.;
    else
        return s;
}

float AWAComputer::get_weight(float v) {
    // Weight should be highest close to 0 and lowest close to 1 and - 1,
    // so that the angle estimate is most accurate in the middle of the arc sine function,
    // and we avoid using the top and bottom where the clamping was observed

    float weight = (1.f - std::abs(v)) * 10;
    if (weight < 1.f)
        return 1.f;  // Avoid division by 0 if all weights are 0
    else
        return weight;
}

float AWAComputer::angle_red_0_180(float x) {
    return trunc_angle( std::asin(x) + 3 * (float)M_PI / 6.f);
}

float AWAComputer::angle_red_180_360(float x) {
    return trunc_angle(-std::asin(x) - 3 * (float)M_PI / 6.f);
}

float AWAComputer::angle_green_120_300(float x) {
    auto y = std::asin(x) - 5 * (float) M_PI / 6.f;
    return (float)trunc_angle(y);
}

float AWAComputer::angle_green_300_120(float x) {
    return trunc_angle(-std::asin(x) + 1 * (float)M_PI / 6.f);
}

float AWAComputer::angle_blue_060_240(float x) {
    return trunc_angle(-std::asin(x) + 5 * (float)M_PI / 6.f);
}

float AWAComputer::angle_blue_240_060(float x) {
    return trunc_angle(std::asin(x) - 1 * (float)M_PI / 6.f);
}

float AWAComputer::get_angle_w(float angle_blue, float angle_green, float angle_red, float blue_w, float green_w,
                               float red_w) {

    if ( (angle_green - angle_blue) > M_PI)
        angle_green -= M_TWOPI;
    else if ( (angle_blue - angle_green) > M_PI)
        angle_green += M_TWOPI;

    if ( (angle_red - angle_green) > M_PI)
        angle_red -= M_TWOPI;
    else if( (angle_green - angle_red) > M_PI)
        angle_red += M_TWOPI;


    return trunc_angle((blue_w * angle_blue + green_w * angle_green + red_w * angle_red) / (blue_w + green_w + red_w));
}

float AWAComputer::trunc_angle(float angle) {
    // Truncate angle to [0; 2pi]
    if (angle < 0)
        return angle + float(M_TWOPI);
    else if (angle > M_TWOPI)
        return angle - float(M_TWOPI);
    else
        return angle;
}


