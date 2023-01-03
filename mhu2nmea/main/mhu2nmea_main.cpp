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

#endif


//#define WIND_SPEED_PULSE_IO 18
//#define WATER_SPEED_PULSE_IO 35  // MHU wind speed pulse 35 - OPTO IN

#define WIND_SPEED_PULSE_IO 15 // MHU wind speed pulse - DI1
#define WATER_SPEED_PULSE_IO 13  // Paddle wheel pulse - DI2

xQueueHandle evt_queue;   // A queue to handle  send events from sensors to N2K

N2KHandler N2Khandler(evt_queue);

#ifdef HAS_ADC
AWAHandler AWAhandler(evt_queue);
#endif

CNTHandler cntHandler;

AWSHandler awsHandler(evt_queue);

SOWHandler sowHandler(evt_queue);

extern "C" [[noreturn]] void app_main(void)
{
    // Initialize event queue
    evt_queue = xQueueCreate(10, sizeof(Event));

    // Start NMEA 2000 task
    N2Khandler.StartTask();

    cntHandler.AddCounterHandler(&awsHandler, WIND_SPEED_PULSE_IO, 4);
    cntHandler.AddCounterHandler(&sowHandler, WATER_SPEED_PULSE_IO, 4);

#ifdef HAS_ADC
    AWAhandler.StartTask();
#endif

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
