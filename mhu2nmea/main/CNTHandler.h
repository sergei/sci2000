#ifndef MHU2NMEA_CNTHANDLER_H
#define MHU2NMEA_CNTHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "Event.hpp"

typedef struct {
    int unit;           // the PCNT unit that originated an interrupt
    uint32_t status;    // information on the event type that caused the interrupt
    int64_t elapsed_us; // Time since last event
}pcnt_evt_t;

class CNTHandler {
public:
    explicit CNTHandler(const xQueueHandle &dataQueue, int pulseGpio, pcnt_unit_t unit);
    void Start();

    [[noreturn]] [[noreturn]] void CounterTask();
private:
    float convertToHz(const pcnt_evt_t &evt);
    int pulseGpio;
    pcnt_unit_t unit;
    const xQueueHandle &m_dataQueue;
};


#endif //MHU2NMEA_CNTHANDLER_H
