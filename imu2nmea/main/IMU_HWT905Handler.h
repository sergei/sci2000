#ifndef IMU2NMEA_IMU_HWT905HANDLER_H
#define IMU2NMEA_IMU_HWT905HANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "IMUCalInterface.h"
#include "MagDeviation.h"
#include <hal/uart_types.h>
#include <driver/uart.h>

class IMU_HWT905Handler  : public IMUCalInterface{
public:
    IMU_HWT905Handler(const xQueueHandle &eventQueue, int tx_io_num, int rx_io_num, uart_port_t uart_num);
    void Start();
    void StoreCalibration() override;
    void EraseCalibration() override;


public: // For use in static functions
    [[noreturn]] void Task();
    void writeToHwt(const uint8_t *b, size_t len) const;
    void onSensorData(uint32_t uiReg, const int16_t *data, uint32_t uiRegNum);

private:
    const xQueueHandle &systemEventQueue;
    const int tx_io_num;
    const int rx_io_num;
    const uart_port_t uart_num;
    MagDeviation magDeviation;
    const int uart_buffer_size = 4 * 1024;
    QueueHandle_t m_uartEventQueue = nullptr;
    static void ProcessInputBytes(uint8_t *data, size_t size);

    void initImu();

    float m_fPitch = 0.0f;
    float m_fRoll = 0.0f;
    float m_fYaw = 0.0f;

};


#endif //IMU2NMEA_IMU_HWT905HANDLER_H
