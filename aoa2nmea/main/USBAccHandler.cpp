#include <esp_log.h>
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <cstring>
#include <sys/param.h>
#include <lwip/def.h>
#include "USBAccHandler.h"

static const char *TAG = "aoa2nmea_USBAccHandler";

USBAccHandler::USBAccHandler(SideTwaiBusInterface &twaiBusSender, int tx_io_num, int rx_io_num, uart_port_t uart_num)
:twaiBusSender(twaiBusSender)
,tx_io_num(tx_io_num)
,rx_io_num(rx_io_num)
,uart_num(uart_num)
,slipPacket(*this, *this)
{
    esp_log_level_set(TAG, ESP_LOG_INFO); // enable DEBUG logs
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
            .baud_rate = 115200,
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
    size_t bytes2read;
    uint8_t dtmp[uart_buffer_size];

    for (;;) {

        if(xQueueReceive(m_uartEventQueue, (void * )&event, (portTickType)portMAX_DELAY)){
            ESP_LOGV(TAG, "uart[%d] event:", uart_num);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    ESP_LOGV(TAG, "[UART DATA]: %d", event.size);
                    bytes2read = MIN(event.size, uart_buffer_size);
                    uart_read_bytes(uart_num, dtmp, bytes2read, 0);
                    for(int i=0; i < event.size; i++){
                        slipPacket.onSlipByteReceived(dtmp[i]);
                    }
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
    ESP_LOGV(TAG, "onTwaiFrameReceived: %ld, %d", id, len);

    unsigned char net_data[TWAI_FRAME_MAX_DLC + 5];
    int net_id = htonl(id);
    memcpy(net_data, &net_id, 4);
    memcpy(net_data + 4, &len, 1);
    for(int i = 0; i < len; i++){
        net_data[5 + i] = buf[i];
    }
    int total_len = 5 + len;

    slipPacket.EncodeAndSendPacket(net_data, total_len);

}

void USBAccHandler::onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) {
    onTwaiFrameReceived(id, len, buf);
}

void USBAccHandler::flush() {
    // Do nothing
}

void USBAccHandler::sendEncodedBytes(unsigned char *buf, unsigned char len) {
    int nsent = uart_write_bytes(uart_num, buf, len);
    ESP_LOGD(TAG, "USB< sent %d bytes", nsent);
    if( nsent != len ){
        ESP_LOGE(TAG, "USB< Error sending %d bytes", len);
    }
}

void USBAccHandler::onPacketReceived(const unsigned char *recvbuf, unsigned char len) {
    int net_id = 0;
    memcpy(&net_id, recvbuf, 4);
    int twai_id = ntohl(net_id);
    unsigned char twai_len = recvbuf[4];

    if( twai_len <= TWAI_FRAME_MAX_DLC ){
        ESP_LOGD(TAG, "USB> received %d bytes: id=%08X len=%02X", len, twai_id, twai_len);
        twaiBusSender.onSideIfcTwaiFrame(twai_id, twai_len, &recvbuf[5]);
    }else{
        ESP_LOGE(TAG, "USB> Invalid DLC %02X", twai_len);
    }
}
