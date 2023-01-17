#include <esp_log.h>
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <cstring>
#include "GPSHandler.h"

static const char *TAG = "imu2nmea_GPSHandler";

GPSHandler::GPSHandler(QueueHandle_t const &eventQueue, int tx_io_num, int rx_io_num)
:eventQueue(eventQueue)
,tx_io_num(tx_io_num)
,rx_io_num(rx_io_num)
{

}

static void gps_task(void *me ) {
    ((GPSHandler *)me)->Task();
}

void GPSHandler::Start() {
    xTaskCreate(
            gps_task,         /* Function that implements the task. */
            "GPSTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            (void *) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr);        /* Used to pass out the created task's handle. */

}

void GPSHandler::Task() {
    ESP_LOGI(TAG, "Opening serial port");
    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_APB,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_io_num, rx_io_num, 0, 0));

    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size,
                                        uart_buffer_size, 10, &uart_queue, 0));

    uint8_t data[128];
    const char* test_str = "This is a test string.\n";
    uart_write_bytes(uart_num, test_str, strlen(test_str));

    for (;;) {
        auto size = sizeof(data) - 1;
        int length = uart_read_bytes(uart_num, data, size, 1000 / portTICK_PERIOD_MS);
        if ( length > 0 ){
            data[length] = '\0';
            ESP_LOGI(TAG, "Got %d bytes [%s]", length, data);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}