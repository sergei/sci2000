#include "esp_log.h"
#include "ADCHandler.h"

static const char *TAG = "mhu2nmea_ADCHandler";

void ADCHandler::Init() {

    // Initialize I2C component
    ESP_ERROR_CHECK(i2cdev_init());

    // Initialize ADC
    ESP_ERROR_CHECK(ads111x_init_desc(&this->dev,
                                      ADS111X_ADDR_SCL,
                                      I2C_NUM_0,
                                      GPIO_NUM_16,
                                      GPIO_NUM_17));

    ESP_ERROR_CHECK(ads111x_set_data_rate(&this->dev, ADS111X_DATA_RATE_64));
    ESP_ERROR_CHECK(ads111x_set_gain(&this->dev, ADS111X_GAIN_6V144));
    ESP_ERROR_CHECK(ads111x_set_mode(&this->dev, ADS111X_MODE_CONTINUOUS));

    // Read configuration back
    ads111x_data_rate_t rate;
    ESP_ERROR_CHECK(ads111x_get_data_rate(&this->dev, &rate));
    ads111x_gain_t gain;
    ESP_ERROR_CHECK(ads111x_get_gain(&this->dev, &gain));
    ads111x_mode_t mode;
    ESP_ERROR_CHECK(ads111x_get_mode(&this->dev, &mode));

    ESP_LOGI(TAG, "rate=%d gain=%d mode=%d", rate, gain, mode);
    ESP_ERROR_CHECK(ads111x_start_conversion(&this->dev));
}

bool ADCHandler::Poll(int16_t *data, int chanNum) {
    const ads111x_mux_t mux[] = {ADS111X_MUX_0_GND, ADS111X_MUX_1_GND, ADS111X_MUX_2_GND, ADS111X_MUX_3_GND};

    for( int i =0; i < chanNum; i++) {
        ESP_ERROR_CHECK(ads111x_set_input_mux(&this->dev, mux[i]));
        ESP_ERROR_CHECK(ads111x_get_value(&this->dev, data + i));
    }

    return true;
}
