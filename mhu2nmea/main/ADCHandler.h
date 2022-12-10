#ifndef MHU2NMEA_ADCHANDLER_H
#define MHU2NMEA_ADCHANDLER_H

#include "ads111x.h"

class ADCHandler {
public:
    void Init();
    bool Poll(int16_t data[], int chanNum);
private:
    i2c_dev_t dev;
};


#endif //MHU2NMEA_ADCHANDLER_H
