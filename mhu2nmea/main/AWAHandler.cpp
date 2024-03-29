#include <cmath>
#include "esp_log.h"
#include "AWAHandler.h"

static const char *TAG = "mhu2nmea_AWAHandler";

void AWAHandler::Init() {

    // Initialize I2C component
    ESP_ERROR_CHECK(i2cdev_init());

    // Initialize ADC
    ESP_ERROR_CHECK(ads111x_init_desc(&this->dev,
                                      ADS111X_ADDR_SCL,
                                      I2C_NUM_0,
                                      GPIO_NUM_16,
                                      GPIO_NUM_17));

    ESP_ERROR_CHECK(ads111x_set_data_rate(&this->dev, ADS111X_DATA_RATE_250));
    ESP_ERROR_CHECK(ads111x_set_gain(&this->dev, ADS111X_GAIN_6V144));
    ESP_ERROR_CHECK(ads111x_set_mode(&this->dev, ADS111X_MODE_SINGLE_SHOT));

    // Read configuration back
    ads111x_data_rate_t rate;
    ESP_ERROR_CHECK(ads111x_get_data_rate(&this->dev, &rate));
    ads111x_gain_t gain;
    ESP_ERROR_CHECK(ads111x_get_gain(&this->dev, &gain));
    ads111x_mode_t mode;
    ESP_ERROR_CHECK(ads111x_get_mode(&this->dev, &mode));

    ESP_LOGI(TAG, "rate=%d gain=%d mode=%d", rate, gain, mode);
}

bool AWAHandler::Poll(int16_t *data, int chanNum) {
    const ads111x_mux_t mux[] = {ADS111X_MUX_0_GND, ADS111X_MUX_1_GND, ADS111X_MUX_2_GND, ADS111X_MUX_3_GND};

    for(int ch_idx =0; ch_idx < chanNum; ch_idx++) {
        ESP_ERROR_CHECK(ads111x_set_input_mux(&this->dev, mux[ch_idx]));
        ESP_ERROR_CHECK(ads111x_start_conversion(&this->dev));

        int ms_count;
        for(ms_count = 0; ms_count < 100 ; ms_count++ ){
            bool isBusy;
            ESP_ERROR_CHECK(ads111x_is_busy(&this->dev, &isBusy));
            if ( ! isBusy ){
                int16_t value;
                ESP_ERROR_CHECK(ads111x_get_value(&this->dev, &value));
                ESP_LOGV(TAG, "Read c[%d]=%04x after %d ms", ch_idx, value, ms_count);
                data[ch_idx] = value;
                break;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS); // 1 mS
        }
        if (ms_count == 3){
            ESP_LOGE(TAG, "Was busy for ch %d", ch_idx);
            return false;
        }
    }

    return true;
}

bool AWAHandler::pollAwa(float &awaRad) {
    int16_t adc_data[4];
    if ( Poll(adc_data, 3) ) {

        auto red = adc_data[0];
        auto green = adc_data[1];
        auto blue = adc_data[2];

        // Estimate amplitude to check if the sensor is connected
        auto raw_a = (red + green + blue);

        if ( raw_a > 300 ){
            // Compute time since last poll
            int64_t now_us = esp_timer_get_time();
            auto dt_sec = ((float)(now_us - last_awa_poll_time_us) / 1000000.0f);
            last_awa_poll_time_us = now_us;

            awaRad = awaComputer.computeAwa(red, green, blue, dt_sec);
            float awaDeg = RAD_2_DEG(awaRad);

            ESP_LOGI(TAG, "AWA,adc_red,%d,adc_green,%d,adc_blue,%d,awa,%.1f", adc_data[0], adc_data[1], adc_data[2], awaDeg);
            return true;
        }
        else{
            ESP_LOGI(TAG, "AWA,adc_red,%d,adc_green,%d,adc_blue,%d,awa,", adc_data[0], adc_data[1], adc_data[2]);
            return false; // Apparently sensor is not connected
        }
    }

    return false;
}

[[noreturn]] void AWAHandler::AWATask() {
    Init();

    for( ;; ){
        float awa;
        bool validAwa = this->pollAwa(awa);
        Event evt = {
            .src = AWA,
            .isValid = validAwa,
            .u { .fValue = awa }
        };
        xQueueSend(eventQueue, &evt, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS); // 100 mS
    }
}

static void awa_task( void *me ) {
    ((AWAHandler *)me)->AWATask();
}

void AWAHandler::StartTask() {
    xTaskCreate(
            awa_task,         /* Function that implements the task. */
            "AWATask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            ( void * ) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr );        /* Used to pass out the created task's handle. */
}
