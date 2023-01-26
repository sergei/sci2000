#ifndef MHU2NMEA_SOWHANDLER_H
#define MHU2NMEA_SOWHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "CNTHandler.h"

static const float  PW_HERTZ_PER_KTS =  4.0; // Hz/Kt
class SOWHandler  : public CounterHandler {
public:
    explicit SOWHandler(const xQueueHandle &dataQueue);
    void onCounted(bool isValid, float Hz) override;

private:
    const xQueueHandle &m_dataQueue;

};


#endif //MHU2NMEA_SOWHANDLER_H
