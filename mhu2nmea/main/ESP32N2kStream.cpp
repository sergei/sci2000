#include <driver/uart.h>
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
    ESP_LOG_BUFFER_CHAR(TAG, data, size);
    return size;
}
