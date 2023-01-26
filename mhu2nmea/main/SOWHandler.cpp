#include "SOWHandler.h"


SOWHandler::SOWHandler(QueueHandle_t const &dataQueue)
        :m_dataQueue(dataQueue)
{

}

void SOWHandler::onCounted(bool isValid, float Hz) {
    float speedKts = Hz / PW_HERTZ_PER_KTS;
    Event dataEvt = {
            .src = SOW,
            .isValid = isValid,
            .u = {.fValue = speedKts}
    };
    xQueueSend(m_dataQueue, &dataEvt, 0);
}
