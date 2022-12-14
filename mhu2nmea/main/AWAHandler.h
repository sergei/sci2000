#ifndef MHU2NMEA_ADCHANDLER_H
#define MHU2NMEA_ADCHANDLER_H

#include "ads111x.h"
#include "Event.hpp"

static const float NOMINAL_AMPLITUDE = 2471;  // Measured ADC output with lab supply set to 6.4 V

class AWAHandler {
public:
    AWAHandler(const xQueueHandle &eventQueue):eventQueue(eventQueue){}
    void StartTask();

    [[noreturn]] [[noreturn]] void AWATask();
private:
    void Init();
    bool pollAwa(float &awaRad);
    bool Poll(int16_t data[], int chanNum);
    i2c_dev_t dev;
    float m_filteredAmplitude = NOMINAL_AMPLITUDE;
    const xQueueHandle &eventQueue;
};


#endif //MHU2NMEA_ADCHANDLER_H
