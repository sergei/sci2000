#include "AWSHandler.h"

AWSHandler::AWSHandler(QueueHandle_t const &dataQueue)
        :m_dataQueue(dataQueue)
{

}

void AWSHandler::onCounted(bool isValid, float Hz) {
    // Now post the value to the central data queue
    float kts = 0;
    if(Hz >= AWS_THR_HZ){
        kts = AWS_A0 + Hz * AWS_B0;
    }
    Event dataEvt = {
            .src = AWS,
            .isValid = isValid,
            .u = {.fValue = kts}
    };
    xQueueSend(m_dataQueue, &dataEvt, 0);
}

