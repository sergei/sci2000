#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/pcnt.h"
#include "N2KHandler.h"
#include "AWAHandler.h"
#include "CNTHandler.h"

#define WIND_SPEED_PULSE_IO 35  // MHU wind speed pulse ( OPTO IN )

xQueueHandle evt_queue;   // A queue to handle  send events from sensors to N2K

N2KHandler N2Khandler(evt_queue);
AWAHandler AWAhandler(evt_queue);
CNTHandler CNThandler(evt_queue, WIND_SPEED_PULSE_IO, PCNT_UNIT_0);  // Use unit 0 to count pulses on

extern "C" [[noreturn]] void app_main(void)
{
    //. Initialize event queue
    evt_queue = xQueueCreate(10, sizeof(Event));

    // Start NMEA 2000 task
    N2Khandler.StartTask();

    // Start sensors tasks
    CNThandler.Start();
    AWAhandler.StartTask();

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
