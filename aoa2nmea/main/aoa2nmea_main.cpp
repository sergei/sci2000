#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "N2KHandler.h"
#include "LEDBlinker.h"
#include "USBAccHandler.h"
#include "Event.hpp"

//#define ENABLE_WIFI
//#define ENABLE_BT

xQueueHandle evt_queue;   // A queue to handle  send events from sensors to N2K

LEDBlinker ledBlinker(GPIO_NUM_2);

N2KHandler n2KHandler(evt_queue, ledBlinker);

USBAccHandler usbAccHandler(n2KHandler, 26, 27, UART_NUM_2);

#ifdef ENABLE_WIFI
#include "N2kWifi.h"
N2kWifi n2kWifi(n2KHandler);
#endif

#ifdef ENABLE_BT
#include "N2kBt.h"
N2kBt n2kBt(n2KHandler);
#endif

static const char *TAG = "aoa2nmea_main";

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

#ifdef ENABLE_BT
    n2kBt.Start();
    n2KHandler.addBusListener((TwaiBusListener *)&n2kBt);
#endif

    n2KHandler.addBusListener(&usbAccHandler);

    ledBlinker.Start();
    usbAccHandler.Start();
    n2KHandler.Start();

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
