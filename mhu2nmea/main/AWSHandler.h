#ifndef MHU2NMEA_AWSHANDLER_H
#define MHU2NMEA_AWSHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "CNTHandler.h"
#include "LowPassFilter.h"

static const float AWS_CUTOFF_FREQ_HZ = 1.f;  // Hertz

class AWSHandler : public CounterHandler {
public:
    explicit AWSHandler(const xQueueHandle &dataQueue);
    void onCounted(bool isValid, float Hz) override;

private:
    const xQueueHandle &m_dataQueue;
// Factors to convert from Hz to KTS
// See https://github.com/sergei/sci2000/issues/2 for details
    constexpr static const float AWS_A0 = 0.99096;  // Intersection
    constexpr static const float AWS_B0 = 0.96745;  // Slope
    constexpr static const float AWS_THR_HZ = 0.04; // Show 0 kts if frequency below this value
};


#endif //MHU2NMEA_AWSHANDLER_H
