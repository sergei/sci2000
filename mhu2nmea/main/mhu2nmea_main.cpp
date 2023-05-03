#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/pcnt.h"

#include "N2KHandler.h"
#include "CNTHandler.h"

#define HAS_ADC
#ifdef HAS_ADC
#include "AWAHandler.h"
#include "AWSHandler.h"
#include "SOWHandler.h"
#include "LEDBlinker.h"

#endif

#define WIND_SPEED_PULSE_IO 13 // MHU wind speed pulse - DI2
#define WATER_SPEED_PULSE_IO 15  // Paddle wheel pulse - DI1

xQueueHandle evt_queue;   // A queue to handle  send events from sensors to N2K

LEDBlinker ledBlinker(GPIO_NUM_2);
N2KHandler N2Khandler(evt_queue, ledBlinker);

#ifdef HAS_ADC
AWAHandler AWAhandler(evt_queue);
#endif

CNTHandler cntHandler;
AWSHandler awsHandler(evt_queue);
SOWHandler sowHandler(evt_queue);

static const char *TAG = "mhu2nmea_main";

extern "C" [[noreturn]] void app_main(void)
{
    ESP_LOGE(TAG,"Git hash:%s",GIT_HASH);

    // Initialize event queue
    evt_queue = xQueueCreate(10, sizeof(Event));

    ledBlinker.Start();

    // Start NMEA 2000 task
    N2Khandler.StartTask();

    cntHandler.AddCounterHandler(&awsHandler, WIND_SPEED_PULSE_IO, 4,  GPIO_FLOATING);
    cntHandler.AddCounterHandler(&sowHandler, WATER_SPEED_PULSE_IO, 4, GPIO_FLOATING);

#ifdef HAS_ADC
    AWAhandler.StartTask();
#endif

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
