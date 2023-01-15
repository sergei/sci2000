#include <esp_log.h>
#include "AWSHandler.h"

static const char *TAG = "mhu2nmea_AWSHandler";

AWSHandler::AWSHandler(QueueHandle_t const &dataQueue)
        :m_dataQueue(dataQueue)
{

}

void AWSHandler::onCounted(bool isValid, float Hz) {
    // Now post the value to the central data queue
    Event dataEvt = {
            .src = AWS,
            .isValid = isValid,
            .u = {.fValue = Hz}  // FIXME convert to KTS
    };
    ESP_LOGI(TAG, "isvalid = %d val=%.2f", dataEvt.isValid, dataEvt.u.fValue );
    xQueueSend(m_dataQueue, &dataEvt, 0);
}

