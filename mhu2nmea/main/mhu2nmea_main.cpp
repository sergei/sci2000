#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/pcnt.h"
#include "N2KHandler.h"
#include "AWAHandler.h"
#include "CNTHandler.h"

static const char *TAG = "mhu2nmea_main";

#define WIND_SPEED_PULSE_IO 35  // MHU wind speed pulse ( OPTO IN )

xQueueHandle evt_queue;   // A queue to handle  events from various tasks

N2KHandler N2Khandler;
AWAHandler AWAhandler(evt_queue);
CNTHandler CNThandler(evt_queue, WIND_SPEED_PULSE_IO, PCNT_UNIT_0);  // Use unit 0 to count pulses on

extern "C" void app_main(void)
{
    /* Initialize PCNT event queue and PCNT functions */
    evt_queue = xQueueCreate(10, sizeof(Event));

    Event evt;
    portBASE_TYPE res;

    CNThandler.Start();
//    AWAhandler.StartTask();
    N2Khandler.StartTask();

    while (1) {
        /* Wait for the event information passed from PCNT's interrupt handler.
         * Once received, decode the event type and print it on the serial monitor.
         */
        res = xQueueReceive(evt_queue, &evt, 1000 / portTICK_PERIOD_MS);
        if (res == pdTRUE) {
            if ( evt.src == AWS_INR){
                float freqHz = CNThandler.getPulseRateHz(evt);
                ESP_LOGI(TAG, "f = %.3f Hz" ,freqHz);
                N2Khandler.SetAws(true, freqHz); // FIXME convert Hz to Kts
            } else if ( evt.src == AWA_POLL){
                if ( evt.u.awa.isValid )
                    ESP_LOGI(TAG,"AWA=%.1f", evt.u.awa.value * 180 / PI);
                else
                    ESP_LOGE(TAG, "Failed to read AWA");
                N2Khandler.SetAwa(evt.u.awa.isValid, evt.u.awa.value);
            }
        }
    }

}
