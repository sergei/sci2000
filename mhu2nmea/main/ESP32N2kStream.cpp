#include "ESP32N2kStream.h"
#include "esp_log.h"

static const char *TAG = "mhu2nmea_ESP32N2kStream";

int ESP32N2kStream::read() {
    return 0;
}

int ESP32N2kStream::peek() {
    return 0;
}

size_t ESP32N2kStream::write(const uint8_t *data, size_t size) {
    for( int i =0 ; i < size; i++){
        if(data[i] == '\n' || len == (sizeof(buff) -1)) {
            buff[len] = '\0';
            ESP_LOGI(TAG, "%s", buff);
            len = 0;
        }else{
            buff[len++] = (char)data[i];
        }
    }
    return size;
}
