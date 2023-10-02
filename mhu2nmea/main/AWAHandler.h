#ifndef MHU2NMEA_ADCHANDLER_H
#define MHU2NMEA_ADCHANDLER_H

#include "ads111x.h"
#include "Event.hpp"
#include "LowPassFilter.h"
#include "AWAComputer.h"

#define RAD_2_DEG(x) ((x) * 180.0 / M_PI)


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
    const xQueueHandle &eventQueue;
    AWAComputer awaComputer;
    int64_t last_awa_poll_time_us = esp_timer_get_time();
};


#endif //MHU2NMEA_ADCHANDLER_H
