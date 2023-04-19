#include "AWSHandler.h"
static const char *TAG = "mhu2nmea_AWSHandler";

AWSHandler::AWSHandler(QueueHandle_t const &dataQueue)
:CounterHandler("AWS",AWS_CUTOFF_FREQ_HZ)
,m_dataQueue(dataQueue)
{

}

void AWSHandler::onCounted(bool isValid, float Hz) {
    // Now post the value to the central data queue
    float kts = 0;
    if(Hz >= AWS_THR_HZ){
        kts = AWS_A0 + Hz * AWS_B0;
        ESP_LOGI(TAG, "AWS_KTS,%.1f", kts);
    }
    Event dataEvt = {
            .src = AWS,
            .isValid = isValid,
            .u = {.fValue = kts}
    };
    xQueueSend(m_dataQueue, &dataEvt, 0);
}

