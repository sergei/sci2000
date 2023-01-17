#include <esp_log.h>
#include "driver/i2c.h"
#include "IMUHandler.h"
#include "Event.hpp"

static const int  I2C_MASTER_NUM = 0;              /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
static const uint32_t I2C_MASTER_FREQ_HZ = 100000; /*!< I2C master clock frequency */
static const int I2C_MASTER_TX_BUF_DISABLE  =  0;  /*!< I2C master doesn't need buffer */
static const int  I2C_MASTER_RX_BUF_DISABLE =  0;  /*!< I2C master doesn't need buffer */
static const int MS_TO_WAIT = 100;

// CMPS12 Internal registers ( see imu2nmea/doc/cmps12.pdf )
static const int REG_COMP_HI = (uint8_t) 0x02;
static const int REG_PITCH = (uint8_t) 0x04;
static const int REG_ROLL = (uint8_t) 0x05;
static const int REG_CAL = (uint8_t) 0x1e;

static const char *TAG = "imu2nmea_IMUHandler";

static void imu_task(void *me ) {
    ((IMUHandler *)me)->Task();
}

IMUHandler::IMUHandler(QueueHandle_t const &eventQueue, int sda_io_num, int scl_io_num, uint8_t i2c_addr)
:eventQueue(eventQueue)
,sda_io_num(sda_io_num)
,scl_io_num(scl_io_num)
,i2c_addr(i2c_addr)
{
}

void IMUHandler::Start() {
    xTaskCreate(
            imu_task,         /* Function that implements the task. */
            "IMUTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            (void *) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr);        /* Used to pass out the created task's handle. */
}

[[noreturn]] void IMUHandler::Task() {
    bool initOk = InitI2C();

    for( ;; ){
        if ( initOk) {

            uint8_t calibrState=0;
            uint16_t comp=0;
            int8_t pitch=0;
            int8_t roll=0;

            esp_err_t err;
            bool isValid = true;
            err = readReg(REG_CAL, calibrState);
            if ( err != ESP_OK)
                isValid = false;
            err = readReg(REG_COMP_HI, comp);
            if ( err != ESP_OK)
                isValid = false;
            err = readReg(REG_PITCH, pitch);
            if ( err != ESP_OK)
                isValid = false;
            err = readReg(REG_ROLL, roll);
            if ( err != ESP_OK)
                isValid = false;

            if (isValid){
                ESP_LOGD(TAG, "cal=%02X comp=%04X pitch=%02X roll=%02X", calibrState, comp, pitch, roll);
            }else{
                ESP_LOGE(TAG, "I2C error %d %s", err, esp_err_to_name(err));
            }

            Event evt = {
                    .src = IMU,
                    .isValid = isValid,
                    .u{
                        .imu {
                            .hdg = (float)comp / 10.f,
                            .pitch = (float) pitch,
                            .roll = (float) roll,
                            .sysCal = (uint8_t)((calibrState >> 6) & 0x03),
                            .gyroCal = (uint8_t)((calibrState >> 4) & 0x03),
                            .accCal = (uint8_t)((calibrState >> 2) & 0x03),
                            .magCal = (uint8_t)((calibrState >> 0) & 0x03),
                        }
                    }
            };

            xQueueSend(eventQueue, &evt, 0);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // FIXME must be driven by N2K rate
    }
}

bool IMUHandler::InitI2C() {
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = sda_io_num,
            .scl_io_num = scl_io_num,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master{
                .clk_speed = I2C_MASTER_FREQ_HZ,
                },
            .clk_flags = 0
    };

    i2c_param_config(i2c_master_port, &conf);

    esp_err_t  err = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if ( err != ESP_OK){
        ESP_LOGE(TAG, "Failed to install driver I2C error %d %s", err, esp_err_to_name(err));
        return false;
    }else{
        return true;
    }
}

esp_err_t IMUHandler::readReg(uint8_t regNum, uint8_t &val) const {
    return i2c_master_write_read_device(I2C_MASTER_NUM, i2c_addr,
                                 &regNum, 1, &val, 1, MS_TO_WAIT / portTICK_RATE_MS);
}

esp_err_t IMUHandler::readReg(uint8_t regNum, int8_t &val) const {
    return i2c_master_write_read_device(I2C_MASTER_NUM, i2c_addr,
                                 &regNum, 1, (uint8_t *)&val, 1, MS_TO_WAIT / portTICK_RATE_MS);
}

esp_err_t IMUHandler::readReg(uint8_t regNum, uint16_t &val) const {
    esp_err_t err;

    uint8_t hi;
    err = readReg(regNum, hi);
    if ( err != ESP_OK)
        return err;

    uint8_t lo;
    err = readReg(regNum + 1, lo);
    if ( err != ESP_OK)
        return err;

    val = (((uint16_t)hi) << 8) | (uint16_t)lo;
    return ESP_OK;
}

