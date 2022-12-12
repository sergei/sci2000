/*
2022 Copyright (c) Sergei Podshivalov  All right reserved.

Author: Sergei Podshivalov

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 Inherited object for use NMEA2000 library for ESP32 Boards.
See also https://github.com/ttlappalainen/NMEA2000[NMEA2000] library.

To use this library, you will also need NMEA2000 library.

The library defines as default Tx pin to GPIO 32 and Rx pint to GPIO 34. You can
change these with defines:

  #define ESP32_CAN_TX_PIN GPIO_NUM_32
  #define ESP32_CAN_RX_PIN GPIO_NUM_34

before including NMEA2000_CAN.h or NMEA2000_esp32_twai.h

 */

#ifndef MHU2NMEA_NMEA2000_ESP32_TWAI_H
#define MHU2NMEA_NMEA2000_ESP32_TWAI_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/twai.h"

#include "NMEA2000.h"

#ifndef ESP32_CAN_TX_PIN
#define ESP32_CAN_TX_PIN GPIO_NUM_32
#endif
#ifndef ESP32_CAN_RX_PIN
#define ESP32_CAN_RX_PIN GPIO_NUM_34
#endif

/**
 * This class implements low level CAN interface for tNMEA2000 using builtin ESP32 TWAI controller (that's how they call CAN)
 * See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html
 */
class NMEA2000_esp32_twai : public tNMEA2000 {
public:
    /// Constructor
    /// \param txPin  Configure this GPIO as TWAI Controller output
    /// \param rxPin  Configure this GPIO as TWAI Controller input
    /// \param twaiMode  Set to TWAI_MODE_NO_ACK for self test or TWAI_MODE_LISTEN_ONLY for listen only, otherwise leave default
    /// \param txQueueLen Size of transmit queue, default 5
    /// \param rxQueueLen Size of receive  queue  default 5
    explicit NMEA2000_esp32_twai(gpio_num_t txPin=ESP32_CAN_TX_PIN,  gpio_num_t rxPin=ESP32_CAN_RX_PIN,
                                 twai_mode_t twaiMode = TWAI_MODE_NORMAL, uint32_t txQueueLen=5, uint32_t rxQueueLen=5);

protected:  // Methods the tNMEA2000 wants us to implement for given hardware
    bool CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent) override;
    bool CANOpen() override;
    bool CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf) override;

private:
    gpio_num_t m_TxPin=ESP32_CAN_TX_PIN;
    gpio_num_t m_RxPin=ESP32_CAN_RX_PIN;
    twai_mode_t m_twaiMode= TWAI_MODE_NORMAL;
    uint32_t m_txQueueLen=5;
    uint32_t m_rxQueueLen=5;
    SemaphoreHandle_t ctrl_task_sem;

public:
    [[noreturn]] [[noreturn]] void CtrlTask();
};


#endif //MHU2NMEA_NMEA2000_ESP32_TWAI_H
