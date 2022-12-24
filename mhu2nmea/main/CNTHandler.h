#ifndef MHU2NMEA_CNTHANDLER_H
#define MHU2NMEA_CNTHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <driver/pcnt.h>

#include "Event.hpp"


class CounterHandler{
public:
    virtual void onCounted(bool isValid, float Hz) = 0;
};

typedef struct {
    int unit;           // the PCNT unit that originated an interrupt
    uint32_t status;    // information on the event type that caused the interrupt
    int64_t elapsed_us; // Time since last event
}pcnt_evt_t;

class CNTHandler {
public:
    explicit CNTHandler();
    bool AddCounterHandler(CounterHandler *handler, int pulse_gpio_num, int16_t pulsesPerInterrupt=1);
    [[noreturn]] [[noreturn]] void CounterTask();
public:
    static int64_t last_timer_values[PCNT_UNIT_MAX];
private:
    void Start();
    static float convertToHz(const pcnt_evt_t &evt, int16_t pulsesPerInterrupt);
    void StartUnit(pcnt_unit_t unit, int pulseGpioNum, int16_t pulsesPerInterrupt);
private:
    bool m_Started = false;
    bool m_IsrInstalled = false;
    int m_unitsUsed = 0;
    CounterHandler *m_CtrHandlers[PCNT_UNIT_MAX]{};
    int16_t m_pulsesPerInterrupt[PCNT_UNIT_MAX]{};
};
// PCNT_UNIT_MAX

#endif //MHU2NMEA_CNTHANDLER_H
