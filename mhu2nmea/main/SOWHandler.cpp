#include "SOWHandler.h"


SOWHandler::SOWHandler(QueueHandle_t const &dataQueue)
        :m_dataQueue(dataQueue)
{

}

void SOWHandler::onCounted(bool isValid, float Hz) {
    // Now post the value to the central data queue
    Event dataEvt = {
            .src = SOW,
            .isValid = isValid,
            .u = {.fValue = Hz}  // FIXME convert to KTS
    };
    xQueueSend(m_dataQueue, &dataEvt, 0);
}
