#ifndef MHU2NMEA_LOWPASSFILTER_H
#define MHU2NMEA_LOWPASSFILTER_H


class LowPassFilter {
public:
    explicit LowPassFilter(float cutoff_frequency);
    float filterAngle(float value, float dt);
    float filter(float value, float dt);
    float getFilteredValue() const { return filtered_value; }
private:
    bool initialized=false;
    float omega_cutoff; // cutoff frequency in radians per second
    float time_constant; // filter time constant
    float filtered_value; // filtered value
};


#endif //MHU2NMEA_LOWPASSFILTER_H
