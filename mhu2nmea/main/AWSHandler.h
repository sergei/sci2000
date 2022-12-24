#ifndef MHU2NMEA_AWSHANDLER_H
#define MHU2NMEA_AWSHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "CNTHandler.h"

class AWSHandler : public CounterHandler {
public:
    explicit AWSHandler(const xQueueHandle &dataQueue);
    void onCounted(bool isValid, float Hz) override;

private:
    const xQueueHandle &m_dataQueue;
};


#endif //MHU2NMEA_AWSHANDLER_H
