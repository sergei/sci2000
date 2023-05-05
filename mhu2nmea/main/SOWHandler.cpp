#include "SOWHandler.h"

static const char *TAG = "mhu2nmea_SOWHandler";

SOWHandler::SOWHandler(QueueHandle_t const &dataQueue)
        :CounterHandler("SOW",SPD_CUTOFF_FREQ_HZ)
        ,m_dataQueue(dataQueue)
{

}

void SOWHandler::onCounted(bool isValid, float Hz) {
    float speedKts = Hz / PW_HERTZ_PER_KTS;
    ESP_LOGI(TAG, "SOW_KTS,%.1f", speedKts);

    Event dataEvt = {
            .src = SOW,
            .isValid = isValid,
            .u = {.fValue = speedKts}
    };
    xQueueSend(m_dataQueue, &dataEvt, 0);
}
