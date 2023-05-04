#ifndef IMU2NMEA_IMUHANDLER_H
#define IMU2NMEA_IMUHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "IMUCalInterface.h"

class IMUHandler : public IMUCalInterface{
public:
    IMUHandler(const xQueueHandle &eventQueue, int sda_io_num, int scl_io_num, uint8_t i2c_addr);
    void Start();
    [[noreturn]] void Task();
    void StoreCalibration() override;
    void EraseCalibration() override;

private:
    bool InitI2C();

    const xQueueHandle &eventQueue;

    int sda_io_num = 16;
    int scl_io_num = 17;
    uint8_t i2c_addr = 0;

    esp_err_t readReg(uint8_t regNum, uint8_t &val) const;
    esp_err_t readReg(uint8_t regNum, uint16_t &val) const;
    esp_err_t readReg(uint8_t regNum, int8_t &val) const;

    void doStoreCalibration();
    void doEraseCalibration();

    volatile bool gotStoreCalCmd = false;
    volatile bool gotEraseCalCmd = false;

    void WriteCommand(const uint8_t *cmd) const;
};


#endif //IMU2NMEA_IMUHANDLER_H
