#ifndef MHU2NMEA_CNTHANDLER_H
#define MHU2NMEA_CNTHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <driver/pcnt.h>
#include <esp_log.h>

#include "Event.hpp"
#include "LowPassFilter.h"

// Report 0 Hz if interval between pulses greater than that.
static const double CNT_REPORT_TIMEOUT_SEC = 4.;

class CounterHandler{
public:
    explicit CounterHandler(const char *name, float lpf_cutoff_frequency)
        :m_name(name),m_lpf(lpf_cutoff_frequency){}
    virtual void onCounted(bool isValid, float filtered_hz) = 0;
    virtual void report(bool isValid, float raw_hz);
    const char *GetName() const { return m_name;}

    void checkTimeout();

private:
    const char *m_name;
    LowPassFilter m_lpf;
    int64_t last_report_time_us = esp_timer_get_time();
};

typedef struct {
    int unit;           // the PCNT unit that originated an interrupt
    uint32_t status;    // information on the event type that caused the interrupt
    int64_t elapsed_us; // Time since last event
}pcnt_evt_t;

class CNTHandler {
public:
    explicit CNTHandler();
    bool AddCounterHandler(CounterHandler *handler, int pulse_gpio_num, int16_t pulsesPerInterrupt=1, gpio_pull_mode_t pullMode=GPIO_FLOATING);
    [[noreturn]] [[noreturn]] void CounterTask();
public:
    static int64_t last_timer_values[PCNT_UNIT_MAX];
private:
    void Start();
    static float convertToHz(const pcnt_evt_t &evt, int16_t pulsesPerInterrupt);
    void StartUnit(pcnt_unit_t unit, int pulseGpioNum, int16_t pulsesPerInterrupt, gpio_pull_mode_t pullMode);
private:
    bool m_Started = false;
    bool m_IsrInstalled = false;
    int m_unitsUsed = 0;
    CounterHandler *m_CtrHandlers[PCNT_UNIT_MAX]{};
    int16_t m_pulsesPerInterrupt[PCNT_UNIT_MAX]{};
    gpio_pull_mode_t m_pullMode=GPIO_FLOATING;
};
// PCNT_UNIT_MAX

#endif //MHU2NMEA_CNTHANDLER_H
