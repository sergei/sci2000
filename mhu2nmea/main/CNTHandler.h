#ifndef MHU2NMEA_CNTHANDLER_H
#define MHU2NMEA_CNTHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "Event.hpp"

class CNTHandler {
public:
    explicit CNTHandler(xQueueHandle &eventQueue, int pulseGpio, pcnt_unit_t unit);
    void Start();
    float getPulseRateHz(const Event &evt);
private:
    int pulseGpio;
    pcnt_unit_t unit;
};


#endif //MHU2NMEA_CNTHANDLER_H
