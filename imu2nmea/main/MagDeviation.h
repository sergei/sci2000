#ifndef IMU2NMEA_MAGDEVIATION_H
#define IMU2NMEA_MAGDEVIATION_H


class MagDeviation {
    // Coefficient obtained on Javelin on May, 18 2023
    const float A0 = 12.2f;    // DC
    const float A1 = 18.0f;    // Amplitude of first harmonic
    const float PHI1 = 132.0f; // Phase of first harmonic

public:
    float correct(float hdgDeg) const;
};


#endif //IMU2NMEA_MAGDEVIATION_H
