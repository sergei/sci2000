#include <N2kWifi.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "Event.hpp"
#include "IMUHandler.h"
#include "N2KHandler.h"
#include "LEDBlinker.h"
#include "GPSHandler.h"

#define ENABLE_WIFI

xQueueHandle evt_queue;   // A queue to handle  send events from sensors to N2K

const int SDA_IO_NUM = 16;
const int SCL_IO_NUM = 17;
static const uint8_t CMPS12_i2C_ADDR = 0xC0 >> 1;  //  CMPS12 I2C address

LEDBlinker ledBlinker(GPIO_NUM_2);
IMUHandler imuHandler(evt_queue, SDA_IO_NUM, SCL_IO_NUM, CMPS12_i2C_ADDR);
N2KHandler n2KHandler(evt_queue, ledBlinker);
GPSHandler gpsHandler(evt_queue, 15, 13);

#ifdef ENABLE_WIFI
N2kWifi n2kWifi(n2KHandler);
#endif

static const char *TAG = "imu2nmea_main";

extern "C" [[noreturn]] void app_main(void)
{
    ESP_LOGE(TAG,"Git hash:%s",GIT_HASH);

    // Initialize event queue
    evt_queue = xQueueCreate(10, sizeof(Event));

#ifdef ENABLE_WIFI
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    n2kWifi.Start();
    n2KHandler.addBusListener(&n2kWifi);
#endif


    ledBlinker.Start();
    n2KHandler.Start();
    gpsHandler.Start();
    imuHandler.Task();

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
