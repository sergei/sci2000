#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "Event.hpp"
#include "IMUHandler.h"
#include "N2KHandler.h"
#include "LEDBlinker.h"
#include "GPSHandler.h"


xQueueHandle evt_queue;   // A queue to handle  send events from sensors to N2K

const int SDA_IO_NUM = 16;
const int SCL_IO_NUM = 17;
static const uint8_t CMPS12_i2C_ADDR = 0xC0 >> 1;  //  CMPS12 I2C address

LEDBlinker ledBlinker(GPIO_NUM_2);
IMUHandler imuHandler(evt_queue, SDA_IO_NUM, SCL_IO_NUM, CMPS12_i2C_ADDR);
N2KHandler n2KHandler(evt_queue, ledBlinker);
GPSHandler gpsHandler(evt_queue, 15, 13);

extern "C" [[noreturn]] void app_main(void)
{
    // Initialize event queue
    evt_queue = xQueueCreate(10, sizeof(Event));
    ledBlinker.Start();
//    n2KHandler.Start();
    gpsHandler.Start();
    imuHandler.Task();

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
