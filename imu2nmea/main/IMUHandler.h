#ifndef IMU2NMEA_IMUHANDLER_H
#define IMU2NMEA_IMUHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class IMUHandler {
public:
    IMUHandler(const xQueueHandle &eventQueue, int sda_io_num, int scl_io_num, uint8_t i2c_addr);
    void Start();
    [[noreturn]] void Task();

private:
    bool InitI2C();

    const xQueueHandle &eventQueue;

    int sda_io_num = 16;
    int scl_io_num = 17;
    uint8_t i2c_addr = 0;

    esp_err_t readReg(uint8_t regNum, uint8_t &val) const;
    esp_err_t readReg(uint8_t regNum, uint16_t &val) const;
    esp_err_t readReg(uint8_t regNum, int8_t &val) const;
};


#endif //IMU2NMEA_IMUHANDLER_H
