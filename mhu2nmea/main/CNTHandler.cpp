#include "CNTHandler.h"
#include "esp_log.h"

#define PCNT_L_LIM_VAL     (-15)

static xQueueHandle evtQueue; // Internal queue to communicate between interrupt and main task
static const char *TAG = "mhu2nmea_CNTHandler";

CNTHandler::CNTHandler()
{
    for( int i = 0; i < PCNT_UNIT_MAX; i++){
        m_CtrHandlers[i] = nullptr;
        m_pulsesPerInterrupt[i] = 1;
    }
}

/* Decode what PCNT's unit originated an interrupt
 * and pass this information together with the event type
 * the main task using a queue.
 */
int64_t CNTHandler::last_timer_values[PCNT_UNIT_MAX];
static void IRAM_ATTR pcnt_intr_handler(void *arg)
{
    int64_t current_timer_value = esp_timer_get_time();

    auto pcnt_unit = (pcnt_unit_t)(int)arg;
    pcnt_evt_t evt = {
            .unit = pcnt_unit,
            .status = 0,
            .elapsed_us = current_timer_value - CNTHandler::last_timer_values[pcnt_unit]
    };
    /* Save the PCNT event type that caused an interrupt
       to pass it to the main program */
    pcnt_get_event_status(pcnt_unit, &evt.status);
    xQueueSendFromISR(evtQueue, &evt, NULL);
    CNTHandler::last_timer_values[pcnt_unit] = current_timer_value;
}

static void counter_task( void *me ) {
    ((CNTHandler *)me)->CounterTask();
}

void CNTHandler::Start() {
    ESP_LOGI(TAG, "Starting counter tasks and interrupts");

    evtQueue = xQueueCreate(10, sizeof(pcnt_evt_t));
    xTaskCreate(
            counter_task,      /* Function that implements the task. */
            "CTRTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            ( void * ) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr );        /* Used to pass out the created task's handle. */


    m_Started = true;
}

void CNTHandler::StartUnit(pcnt_unit_t unit, int pulseGpio, int16_t pulsesPerInterrupt) {
    ESP_LOGI(TAG, "Starting counting pulses on GPIO%d unit %d, %d ppi", pulseGpio, unit, pulsesPerInterrupt);

    /* Initialize PCNT functions:
     *  - configure and initialize PCNT
     *  - set up the input filter
     *  - set up the counter events to watch
     */

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
            .counter_h_lim = pulsesPerInterrupt,
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

    // Disable pull up and pull down, otherwise it interferes with Engine Top Hat clamper
    // It's important to call it here otherwise some code up above will override this setting and
    // set it to pull up.
    gpio_set_pull_mode((gpio_num_t)pulseGpio, GPIO_FLOATING);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    /* Install interrupt service and add isr callback handler */
    if(! m_IsrInstalled ){
        pcnt_isr_service_install(0);
        m_IsrInstalled = true;
    }

    pcnt_isr_handler_add(unit, pcnt_intr_handler, (void *) unit);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);
}


float CNTHandler::convertToHz(const pcnt_evt_t &evt, int16_t pulsesPerInterrupt) {
    if (evt.status & PCNT_EVT_H_LIM) {
        // Convert to knots
        float freqHz = 1.f / (float)(evt.elapsed_us) * 1000000.f * (float)pulsesPerInterrupt;
        ESP_LOGD(TAG, "H_LIM EVT unit=%d elapsed time=%lld us ppi=%d f=%.3fHz" ,evt.unit, evt.elapsed_us, pulsesPerInterrupt, freqHz);
        return freqHz;
    }

    return -1;
}

[[noreturn]] void CNTHandler::CounterTask() {
    pcnt_evt_t evt;
    ESP_LOGI(TAG, "Counter tasks started");
    while (true) {
        // Wait for at least 10 seconds, if nothing came then report 0
        portBASE_TYPE res = xQueueReceive(evtQueue, &evt, 10000 / portTICK_PERIOD_MS);

        int unit = evt.unit;

        float hz = 0;
        if (res == pdTRUE) {
            hz = convertToHz(evt, m_pulsesPerInterrupt[unit]);
        }
        // Report valid measurement of 0 Hz even if no pulses were detected. Otherwise we see -- on the screen
        // TODO maybe we can read voltage and deside if it's properly connected?
        m_CtrHandlers[unit]->report(true, hz);
    }

}

bool CNTHandler::AddCounterHandler(CounterHandler *handler, int pulseGpioNum, int16_t pulsesPerInterrupt) {
    if ( m_unitsUsed < PCNT_UNIT_MAX){
        m_CtrHandlers[m_unitsUsed] = handler;
        m_pulsesPerInterrupt[m_unitsUsed] = pulsesPerInterrupt;
        if ( ! m_Started ){
            Start();
        }
        StartUnit((pcnt_unit_t)m_unitsUsed, pulseGpioNum, pulsesPerInterrupt);
        m_unitsUsed++;
        return true;
    }else{
        ESP_LOGE(TAG,"No more counting units available");
        return false;
    }

}

void CounterHandler::report(bool isValid, float raw_hz) {
    float filtered_hz = 0.f;
    if ( isValid ){
        // Compute time since last poll
        int64_t now_us = esp_timer_get_time();
        auto dt_sec = ((float)(now_us - last_poll_time_us) / 1000000.0f);
        last_poll_time_us = now_us;

        // Filter the raw frequency
        filtered_hz = m_lpf.filter(raw_hz, dt_sec);
        ESP_LOGI(TAG,"%s,dt_sec,%.3f,raw_hz,%.2f,hz,%.2f", m_name, dt_sec, raw_hz, filtered_hz);
    }else{
        ESP_LOGE(TAG,"%s,dt_sec,,raw_hz,,hz,", m_name);
    }
    onCounted(isValid, filtered_hz);
}
