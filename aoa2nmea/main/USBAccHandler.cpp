#include <esp_log.h>
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <cstring>
#include "USBAccHandler.h"

static const char *TAG = "aoa2nmea_USBAccHandler";

USBAccHandler::USBAccHandler(SideTwaiBusInterface &twaiBusSender, int tx_io_num, int rx_io_num, uart_port_t uart_num)
:twaiBusSender(twaiBusSender)
,tx_io_num(tx_io_num)
,rx_io_num(rx_io_num)
,uart_num(uart_num)
{

}


static void gps_task(void *me ) {
    ((USBAccHandler *)me)->Task();
}

void USBAccHandler::Start() {
    xTaskCreate(
            gps_task,         /* Function that implements the task. */
            "GPSTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            (void *) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr);        /* Used to pass out the created task's handle. */

}

void USBAccHandler::Task() {
    ESP_LOGI(TAG, "Opening serial port");
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
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size,
                                        uart_buffer_size, 10, &m_uartEventQueue, 0));


    uart_event_t event;
    uint8_t dtmp[uart_buffer_size];

    for (;;) {

        if(xQueueReceive(m_uartEventQueue, (void * )&event, (portTickType)portMAX_DELAY)){
            ESP_LOGD(TAG, "uart[%d] event:", uart_num);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    ESP_LOGD(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(uart_num, dtmp, event.size, portMAX_DELAY);
                    break;
                    //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(uart_num);
                    xQueueReset(m_uartEventQueue);
                    break;
                    //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(uart_num);
                    xQueueReset(m_uartEventQueue);
                    break;
                    //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                    //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                    //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                    //UART_PATTERN_DET
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
}

void USBAccHandler::onTwaiFrameReceived(unsigned long id, unsigned char len, const unsigned char *buf) {

}

void USBAccHandler::onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) {

}

void USBAccHandler::flush() {

}

