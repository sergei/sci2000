#ifndef MHU2NMEA_ADCHANDLER_H
#define MHU2NMEA_ADCHANDLER_H

#include "ads111x.h"
static const float NOMINAL_AMPLITUDE = 2471;  // Measured ADC output with lab supply set to 6.4 V
static const float PI = 3.1415926535;

class ADCHandler {
public:
    void Init();
    bool pollAwa(float &awaRad);
private:
    bool Poll(int16_t data[], int chanNum);
    i2c_dev_t dev;
    float m_filteredAmplitude = NOMINAL_AMPLITUDE;
};


#endif //MHU2NMEA_ADCHANDLER_H
