#include <esp_log.h>
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <cstring>
#include "GPSHandler.h"
#include "minmea.h"
#include "Event.hpp"
#include "UbxParser.h"

static const char *TAG = "imu2nmea_GPSHandler";

GPSHandler::GPSHandler(QueueHandle_t const &eventQueue, int tx_io_num, int rx_io_num, uart_port_t uart_num)
: m_gpsParser(eventQueue, m_ubxParser)
, tx_io_num(tx_io_num)
, rx_io_num(rx_io_num)
, uart_num(uart_num)
,m_ubxParser(*this, m_gpsParser)
{

}

GpsParser::GpsParser(QueueHandle_t const &systemEventQueue, UbxParser &ubxParser)
        :systemEventQueue(systemEventQueue)
        ,m_ubxParser(ubxParser)
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

    bool gotUbx = false;
    bool ubxInitialized = false;
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
                    m_gpsParser.ProcessInputBytes(dtmp, event.size);
                    gotUbx = true;
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
            if( gotUbx && !ubxInitialized ){
                initUbx();
                ubxInitialized = true;
            }
        }
    }
}

void GPSHandler::writeToUbx(const uint8_t *b, size_t len) {
    uart_write_bytes(uart_num,  b, len);
}

void GPSHandler::initUbx() {
    UBX_message_t msg;

    // Disable GSV
    msg.CFG_MSG.msgClass = 0xF0;
    msg.CFG_MSG.msgID = 0x03;
    msg.CFG_MSG.rate = 0;
    m_ubxParser.send_message(CLASS_CFG, CFG_MSG, msg, 3);

    // Disable GSA
    msg.CFG_MSG.msgClass = 0xF0;
    msg.CFG_MSG.msgID = 0x02;
    msg.CFG_MSG.rate = 0;
    m_ubxParser.send_message(CLASS_CFG, CFG_MSG, msg, 3);

    // Disable VTG
    msg.CFG_MSG.msgClass = 0xF0;
    msg.CFG_MSG.msgID = 0x05;
    msg.CFG_MSG.rate = 0;
    m_ubxParser.send_message(CLASS_CFG, CFG_MSG, msg, 3);

    // Disable GLL
    msg.CFG_MSG.msgClass = 0xF0;
    msg.CFG_MSG.msgID = 0x01;
    msg.CFG_MSG.rate = 0;
    m_ubxParser.send_message(CLASS_CFG, CFG_MSG, msg, 3);

    // Enable GGA
    msg.CFG_MSG.msgClass = 0xF0;
    msg.CFG_MSG.msgID = 0x00;
    msg.CFG_MSG.rate = 1;
    m_ubxParser.send_message(CLASS_CFG, CFG_MSG, msg, 3);

    // Enable RMC
    msg.CFG_MSG.msgClass = 0xF0;
    msg.CFG_MSG.msgID = 0x04;
    msg.CFG_MSG.rate = 1;
    m_ubxParser.send_message(CLASS_CFG, CFG_MSG, msg, 3);

    // Set update rate to 5Hz
    msg.CFG_RATE.measRate = 200;
    msg.CFG_RATE.navRate = 1;
    msg.CFG_RATE.timeRef = 0;
    m_ubxParser.send_message(CLASS_CFG, CFG_RATE, msg, 6);

}

void GpsParser::ProcessInputBytes(const uint8_t *buff, size_t size) {
    // Debug print
    printHex(buff, size);

    // Pass to UBX parser
    for( int i=0; i < size; i++){
        m_ubxParser.read(buff[i]);
    }

    // Process NMEA 0183
    for(int i = 0; i < size; i++){
        uint8_t ch = buff[i];
        if( ch == '\r'){
            m_lineBuffer[m_lineBufferIdx] = '\0';
            ParseNmea0183Line(reinterpret_cast<const char *>(m_lineBuffer));
        }else if ( ch == '$'){
            m_lineBufferIdx = 0;
            m_lineBuffer[m_lineBufferIdx] = ch;
            m_lineBufferIdx++;
        }else if (m_lineBufferIdx < sizeof(m_lineBuffer) - 1){
            m_lineBuffer[m_lineBufferIdx] = ch;
            m_lineBufferIdx++;
        }else{
            m_lineBufferIdx = 0;  // Start over
        }
    }
}

void GpsParser::printHex(const uint8_t *buff, size_t size) {
    char acBuff[10];
    char str[4 * 256] = "";
    for( int j = 0; j < size; j++){
        snprintf(acBuff, sizeof(acBuff)-1, "%02X ", buff[j]);
        strcat(str, acBuff);
    }
    ESP_LOGD(TAG, "HEX [%s]", str);
}

void GpsParser::ParseNmea0183Line(const char *lineToParse) {
//    ESP_LOGI(TAG, "[%s]", lineToParse);

    switch (minmea_sentence_id(lineToParse, false)) {
        case MINMEA_SENTENCE_RMC: {
            Event evt = {
                    .src = GPS_DATA_RMC,
                    .isValid = true,
                    .u = {}
            };
            struct minmea_sentence_rmc &frame = evt.u.gps.rmc;
            if (minmea_parse_rmc(&frame, lineToParse)) {
                xQueueSend(systemEventQueue, &evt, 0);
            }
        } break;
        case MINMEA_SENTENCE_GGA: {
            Event evt = {
                    .src = GPS_DATA_GGA,
                    .isValid = true,
                    .u = {}
            };
            struct minmea_sentence_gga &frame = evt.u.gps.gga;
            if (minmea_parse_gga(&frame, lineToParse)) {
                xQueueSend(systemEventQueue, &evt, 0);
            }
        } break;
        default:
            break;
    }
}

void GpsParser::onUbxMsg(uint8_t cls, uint8_t type, UBX_message_t msg) {

}
