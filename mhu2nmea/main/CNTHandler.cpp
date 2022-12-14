#include <driver/pcnt.h>
#include "CNTHandler.h"
#include "esp_log.h"

#define PCNT_H_LIM_VAL      4   // Interrupt on every four pulses
#define PCNT_L_LIM_VAL     (-15)

static xQueueHandle evtQueue; // Internal queue to communicate between interrupt and main task
static const char *TAG = "mhu2nmea_CNTHandler";

CNTHandler::CNTHandler( const xQueueHandle &dataQueue, int pulseGpio, pcnt_unit_t unit)
:pulseGpio(pulseGpio)
,unit(unit)
,m_dataQueue(dataQueue)
{
}

/* Decode what PCNT's unit originated an interrupt
 * and pass this information together with the event type
 * the main task using a queue.
 */
static void IRAM_ATTR pcnt_intr_handler(void *arg)
{
    static int64_t last_timer_value = -1;
    int64_t current_timer_value = esp_timer_get_time();

    auto pcnt_unit = (pcnt_unit_t)(int)arg;
    pcnt_evt_t evt = {
            .unit = pcnt_unit,
            .status = 0,
            .elapsed_us = current_timer_value - last_timer_value
    };
    /* Save the PCNT event type that caused an interrupt
       to pass it to the main program */
    pcnt_get_event_status(pcnt_unit, &evt.status);
    xQueueSendFromISR(evtQueue, &evt, NULL);
    last_timer_value = current_timer_value;
}

static void counter_task( void *me ) {
    ((CNTHandler *)me)->CounterTask();
}

/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
void CNTHandler::Start() {
    evtQueue = xQueueCreate(10, sizeof(pcnt_evt_t));
    xTaskCreate(
            counter_task,      /* Function that implements the task. */
            "CTRTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            ( void * ) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr );        /* Used to pass out the created task's handle. */


    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
            // Set PCNT input signal and control GPIOs
            .pulse_gpio_num = pulseGpio,
            .ctrl_gpio_num = PCNT_PIN_NOT_USED,  // Don't use control input

            .lctrl_mode = PCNT_CHANNEL_LEVEL_ACTION_KEEP, // Should not matter
            .hctrl_mode = PCNT_CHANNEL_LEVEL_ACTION_KEEP, // Should not matter

            // What to do on the positive / negative edge of pulse input?
            .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
            .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
            // Set the maximum and minimum limit values to watch
            .counter_h_lim = PCNT_H_LIM_VAL,
            .counter_l_lim = PCNT_L_LIM_VAL,

            .unit = unit,
            .channel = PCNT_CHANNEL_0,
    };

    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(unit, 100);
    pcnt_filter_enable(unit);
    pcnt_event_enable(unit, PCNT_EVT_H_LIM);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    /* Install interrupt service and add isr callback handler */
    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(unit, pcnt_intr_handler, (void *) unit);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);

}

float CNTHandler::convertToHz(const pcnt_evt_t &evt) {
    int16_t count = 0;
    pcnt_get_counter_value(unit, &count);
    ESP_LOGV(TAG, "Event PCNT unit[%d]; cnt: %d", evt.unit, count);
    if (evt.status & PCNT_EVT_H_LIM) {
        // Convert to knots
        float freqHz = 1.f / (float)evt.elapsed_us * PCNT_H_LIM_VAL * 1000000.f;
        ESP_LOGV(TAG, "H_LIM EVT  elapsed time = %lld us f = %.3f Hz" , evt.elapsed_us, freqHz);
        return freqHz;
    }

    return -1;
}

[[noreturn]] void CNTHandler::CounterTask() {
    pcnt_evt_t evt;
    while (true) {
        // Wait for at least 10 seconds, if nothing came then report 0
        portBASE_TYPE res = xQueueReceive(evtQueue, &evt, 10000 / portTICK_PERIOD_MS);
        float hz = 0;
        if (res == pdTRUE) {
            hz = convertToHz(evt);
        }

        // Now post the value to the central data queue
        Event dataEvt = {
                .src = AWS,
                .isValid = hz > -0.5,
                .u = {.fValue = hz}
        };
        xQueueSend(m_dataQueue, &dataEvt, 0);
    }

}
