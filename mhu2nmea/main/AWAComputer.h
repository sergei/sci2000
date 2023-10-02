#ifndef MHU2NMEA_AWACOMPUTER_H
#define MHU2NMEA_AWACOMPUTER_H

#include "LowPassFilter.h"

class AWAComputer {

public:
    float computeAwa(int16_t r, int16_t g, int16_t b, float dt_sec) ;
private:
    static float trunc_angle(float angle);

    constexpr static const float AWA_CUTOFF_FREQ_HZ = 1.f;  // Hertz
    LowPassFilter m_awaFilter = LowPassFilter(AWA_CUTOFF_FREQ_HZ);

    static float scale_adc(int16_t adc, float ampl);
    static float get_weight(float v);

    // Partial arc sin functions
    static float angle_red_0_180(float x);
    static float angle_red_180_360(float x);

    static float angle_green_120_300(float x);
    static float angle_green_300_120(float x);

    static float angle_blue_060_240(float x);
    static float angle_blue_240_060(float x);

    static float  get_angle_w(float angle_blue, float  angle_green, float  angle_red, float  blue_w, float  green_w, float  red_w);
};


#endif //MHU2NMEA_AWACOMPUTER_H
