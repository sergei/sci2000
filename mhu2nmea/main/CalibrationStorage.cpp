#include <esp_log.h>
#include <nvs_flash.h>
#include "CalibrationStorage.h"

static const char *TAG = "mhu2nmea_CalibrationStorage";

void CalibrationStorage::ReadCalibration(float &awaCorrRad, float &awsFactor) {
    int16_t awaCorr= DEFAULT_AWA_CORR, awsCorr = DEFAULT_AWS_CORR;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, NVS_KEY_AWA, &awaCorr)) != ESP_OK ){
            awaCorr = DEFAULT_AWA_CORR;
            ESP_LOGI(TAG, "No AWA calibration found (%s) using default value %d", esp_err_to_name(err), awaCorr);
        }else{
            ESP_LOGI(TAG, "Read AWA calibration %d ", awaCorr);
        }
        if ( (err=nvs_get_i16(handle, NVS_KEY_AWS, &awsCorr)) != ESP_OK ){
            awsCorr = DEFAULT_AWS_CORR;
            ESP_LOGI(TAG, "No AWS calibration found (%s) using default value %d", esp_err_to_name(err), awaCorr);
        }else{
            ESP_LOGI(TAG, "Read AWS calibration %d ", awsCorr);
        }
        closeNvs(handle);
    }

    awaCorrRad = (float)(awaCorr) * AWA_CAL_SCALE;
    awsFactor = (float)(awsCorr) * AWS_CAL_SCALE;
}

void CalibrationStorage::UpdateAwaCalibration(int16_t deltaAwaCorr) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        int16_t awaCorr;
        // Read current value
        if (nvs_get_i16(handle, NVS_KEY_AWA, &awaCorr) != ESP_OK ){
            awaCorr = DEFAULT_AWA_CORR;
        }

        // Update with received delta
        auto newAwaCorr = (int16_t )(awaCorr + deltaAwaCorr);

        // Store updated value
        esp_err_t err = nvs_set_i16(handle, NVS_KEY_AWA, newAwaCorr);
        ESP_LOGI(TAG, "AWA calibration %d = %d + %d storied %s", newAwaCorr, awaCorr, deltaAwaCorr, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "AWA calibration committed %s", err == ESP_OK ? "OK" : "Failed");
        closeNvs(handle);
    }
}

void CalibrationStorage::UpdateAwsCalibration(int16_t awsCorrDelta) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        int16_t awsCorr;

        // Read current value
        if ( nvs_get_i16(handle, NVS_KEY_AWS, &awsCorr) != ESP_OK ){
            awsCorr = DEFAULT_AWS_CORR;
        }
        // Update with received delta
        auto newAwsCorr = (int16_t)(((float)awsCorr*AWA_CAL_SCALE * (float)awsCorrDelta*AWA_CAL_SCALE) / AWA_CAL_SCALE);

        esp_err_t err = nvs_set_i16(handle, NVS_KEY_AWS, newAwsCorr);
        ESP_LOGI(TAG, "AWS calibration %d = %d * %d storied %s", newAwsCorr, awsCorr, awsCorrDelta, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "AWS calibration committed %s", err == ESP_OK ? "OK" : "Failed");
        closeNvs(handle);
    }
}

nvs_handle_t CalibrationStorage::openNvs() {

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "Formatting  NVS storage (%d)-%s", err, esp_err_to_name(err));
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Open
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return 0;
    } else {
        return my_handle;
    }
}

void CalibrationStorage::closeNvs(nvs_handle_t handle) {
    nvs_close(handle);
}
