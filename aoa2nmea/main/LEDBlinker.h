#ifndef IMU2NMEA_LEDBLINKER_H
#define IMU2NMEA_LEDBLINKER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const int SHORT_BLINK = 100;
static const int LONG_BLINK = 1000;


class LEDBlinker {
public:
    explicit LEDBlinker(gpio_num_t  ledGpio);
    void Start();
    [[noreturn]] void Task();
    void SetBusState(bool busIsOk) {m_busIsOk = busIsOk;};
private:
    const gpio_num_t  m_ledGpio;
    bool m_busIsOk = false;
};


#endif //IMU2NMEA_LEDBLINKER_H
