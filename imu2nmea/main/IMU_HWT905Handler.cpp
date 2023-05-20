#include <esp_log.h>
#include "IMU_HWT905Handler.h"
#include "wit_c_sdk/wit_c_sdk.h"

#include "Event.hpp"

static const char *TAG = "imu2nmea_IMU_HWT905Handler";

static IMU_HWT905Handler *imuHWT905Handler = nullptr;

IMU_HWT905Handler::IMU_HWT905Handler(QueueHandle_t const &eventQueue, int tx_io_num, int rx_io_num, uart_port_t uart_num)
: systemEventQueue(eventQueue), tx_io_num(tx_io_num), rx_io_num(rx_io_num), uart_num(uart_num)
{

}

static void imu_task(void *me ) {
    ((IMU_HWT905Handler *)me)->Task();
}

static void SensorUartSend(uint8_t *p_data, uint32_t uiSize){
    imuHWT905Handler->writeToHwt(p_data, uiSize);
}

static void SensorDataUpdate(uint32_t uiReg, uint32_t uiRegNum){
    ESP_LOGD(TAG, "SensorDataUpdate: 0x%02x, %d", uiReg, uiRegNum);
    imuHWT905Handler -> onSensorData(uiReg, sReg, uiRegNum);
}

static void Delayms(uint16_t ucMs){
    vTaskDelay(ucMs / portTICK_PERIOD_MS);
}


void IMU_HWT905Handler::Start() {
    imuHWT905Handler = this;
    xTaskCreate(
            imu_task,         /* Function that implements the task. */
            "IMU_HWT905",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            (void *) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr);        /* Used to pass out the created task's handle. */
}

void IMU_HWT905Handler::Task() {
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

    bool imuDetected = false;
    bool imuInitialized = false;

    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitSerialWriteRegister(SensorUartSend);
    WitRegisterCallBack(SensorDataUpdate);
    WitDelayMsRegister(Delayms);

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
                    ProcessInputBytes(dtmp, event.size);
                    imuDetected = true;
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
            if(imuDetected && !imuInitialized ){
                initImu();
                imuInitialized = true;
            }
        }
    }
}

void IMU_HWT905Handler::ProcessInputBytes(uint8_t *data, size_t size) {
//    ESP_LOG_BUFFER_HEX(TAG, data, size);
    for(int i = 0; i < size; i++){
        WitSerialDataIn(data[i]);
    }
}

void IMU_HWT905Handler::writeToHwt(const uint8_t *b, size_t len) const {
    uart_write_bytes(uart_num, (const char *)b, len);
}

void IMU_HWT905Handler::initImu() {

}

void IMU_HWT905Handler::StoreCalibration() {

}

void IMU_HWT905Handler::EraseCalibration() {

}

void IMU_HWT905Handler::onSensorData(uint32_t uiReg, const int16_t *data, uint32_t uiRegNum) {
    switch (uiReg) {
        case Roll:
            m_fRoll = (float)(data[Roll]) / 32768.f * 180.f;
            m_fPitch = (float)(data[Pitch]) / 32768.f * 180.f;
            m_fYaw = (float)(data[Yaw]) / 32768.f * 180.f;
            // Change coordinate system from NED to ENU
            m_fYaw = 180 - m_fYaw;
            // Apply magnetic deviation correction
            m_fYaw = magDeviation.correct(m_fYaw);
            ESP_LOGI(TAG, "Roll: %f (0x%04X), Pitch: %f (0x%04X), Yaw: %f (0x%04X)",
                     m_fRoll, data[Roll] & 0x0000FFFF,
                     m_fPitch, data[Pitch] & 0x0000FFFF,
                     m_fYaw, data[Yaw] & 0x0000FFFF);

            Event evt = {
                    .src = IMU,
                    .isValid = true,
                    .u{
                            .imu {
                                    .hdg = (float)m_fYaw,
                                    .pitch = (float) m_fPitch,
                                    .roll = (float) m_fRoll,
                                    .calibrState = 0xff,
                            }
                    }
            };
            xQueueSend(systemEventQueue, &evt, 0);


            break;

    }
}
