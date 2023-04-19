#ifndef MHU2NMEA_ADCHANDLER_H
#define MHU2NMEA_ADCHANDLER_H

#include "ads111x.h"
#include "Event.hpp"
#include "LowPassFilter.h"

#define RAD_2_DEG(x) ((x) * 180.0 / M_PI)

static const float MHU_AMPL_CUTOFF_FREQ_HZ = 0.1f;  // Amplitude is not supposed to change fast
static const float AWA_CUTOFF_FREQ_HZ = 1.f;  // Hertz

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
    LowPassFilter m_amplitudeFilter = LowPassFilter(MHU_AMPL_CUTOFF_FREQ_HZ);
    LowPassFilter m_awaFilter = LowPassFilter(AWA_CUTOFF_FREQ_HZ);
    int64_t last_awa_poll_time_us = esp_timer_get_time();
};


#endif //MHU2NMEA_ADCHANDLER_H
