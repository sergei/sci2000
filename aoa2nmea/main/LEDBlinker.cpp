#include <driver/gpio.h>
#include <esp_log.h>
#include "LEDBlinker.h"

static const char *TAG = "imu2nmea_LEDBlinker";

LEDBlinker::LEDBlinker(gpio_num_t  ledGpio)
:m_ledGpio(ledGpio)
{

}

static void task( void *me ) {
    ((LEDBlinker *)me)->Task();
}

void LEDBlinker::Start() {
    xTaskCreate(
            task,         /* Function that implements the task. */
            "LEDTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            ( void * ) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr );        /* Used to pass out the created task's handle. */
}

[[noreturn]] void LEDBlinker::Task() {
    ESP_LOGI(TAG, "Starting LED task to blink LED %d", m_ledGpio);

    gpio_reset_pin(m_ledGpio);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(m_ledGpio, GPIO_MODE_OUTPUT);

    while (true){
        if ( m_busIsOk ){
            // Long blinks
            gpio_set_level(m_ledGpio, 0);
            vTaskDelay(LONG_BLINK / portTICK_PERIOD_MS);
            gpio_set_level(m_ledGpio, 1);
            vTaskDelay(LONG_BLINK / portTICK_PERIOD_MS);
        }else{
            // Short blinks
            gpio_set_level(m_ledGpio, 0);
            vTaskDelay(SHORT_BLINK / portTICK_PERIOD_MS);
            gpio_set_level(m_ledGpio, 1);
            vTaskDelay(SHORT_BLINK / portTICK_PERIOD_MS);
        }
    }
}

