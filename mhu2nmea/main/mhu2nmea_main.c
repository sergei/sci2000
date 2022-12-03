#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_log.h"
#include "driver/pcnt.h"

static const char *TAG = "mhu2nmea";

#define PCNT_H_LIM_VAL      4   // Interrupt on every four pulses
#define PCNT_L_LIM_VAL     (-15)
#define WIND_SPEED_PULSE_IO 35  // MHU wind speed pulse ( OPTO IN )

xQueueHandle pcnt_evt_queue;   // A queue to handle pulse counter events
/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
    int64_t elapsed_us; // Time since last event
} pcnt_evt_t;

/* Decode what PCNT's unit originated an interrupt
 * and pass this information together with the event type
 * the main program using a queue.
 */
static void IRAM_ATTR pcnt_example_intr_handler(void *arg)
{
    static int64_t last_timer_value = -1;
    int64_t current_timer_value = esp_timer_get_time();

    int pcnt_unit = (int)arg;
    pcnt_evt_t evt;
    evt.unit = pcnt_unit;
    evt.elapsed_us = current_timer_value - last_timer_value;
    /* Save the PCNT event type that caused an interrupt
       to pass it to the main program */
    pcnt_get_event_status(pcnt_unit, &evt.status);
    xQueueSendFromISR(pcnt_evt_queue, &evt, NULL);
    last_timer_value = current_timer_value;
}

/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
static void pcnt_example_init(int unit)
{
    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
            // Set PCNT input signal and control GPIOs
            .pulse_gpio_num = WIND_SPEED_PULSE_IO,
            .ctrl_gpio_num = PCNT_PIN_NOT_USED,  // Don't use control input
            .channel = PCNT_CHANNEL_0,
            .unit = unit,
            // What to do on the positive / negative edge of pulse input?
            .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
            .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
            // Set the maximum and minimum limit values to watch
            .counter_h_lim = PCNT_H_LIM_VAL,
            .counter_l_lim = PCNT_L_LIM_VAL,
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
    pcnt_isr_handler_add(unit, pcnt_example_intr_handler, (void *)unit);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);
}


void app_main(void)
{
    int pcnt_unit = PCNT_UNIT_0;

    /* Initialize PCNT event queue and PCNT functions */
    pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
    pcnt_example_init(pcnt_unit);

    int16_t count = 0;
    pcnt_evt_t evt;
    portBASE_TYPE res;
    while (1) {
        /* Wait for the event information passed from PCNT's interrupt handler.
         * Once received, decode the event type and print it on the serial monitor.
         */
        res = xQueueReceive(pcnt_evt_queue, &evt, 1000 / portTICK_PERIOD_MS);
        if (res == pdTRUE) {
            pcnt_get_counter_value(pcnt_unit, &count);
            ESP_LOGI(TAG, "Event PCNT unit[%d]; cnt: %d", evt.unit, count);
            if (evt.status & PCNT_EVT_H_LIM) {
                // Convert to knots
                float freqHz = 1.f / (float)evt.elapsed_us * PCNT_H_LIM_VAL * 1000000.f;
                ESP_LOGI(TAG, "H_LIM EVT  elapsed time = %lld us f = %.3f Hz" , evt.elapsed_us, freqHz);
            }
        } else {
            pcnt_get_counter_value(pcnt_unit, &count);
            ESP_LOGI(TAG, "Current counter value :%d", count);
        }
    }

}
