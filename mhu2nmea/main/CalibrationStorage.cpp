#include <esp_log.h>
#include <nvs_flash.h>
#include "CalibrationStorage.h"

static const char *TAG = "mhu2nmea_CalibrationStorage";

void CalibrationStorage::ReadAwaCalibration(float &angleCorrRad) {
    int16_t awaCorr= DEFAULT_ANGLE_CORR;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, NVS_KEY_AWA, &awaCorr)) != ESP_OK ){
            awaCorr = DEFAULT_ANGLE_CORR;
            ESP_LOGI(TAG, "No AWA calibration found (%s) using default value %d", esp_err_to_name(err), awaCorr);
        }else{
            ESP_LOGI(TAG, "Read AWA calibration %d ", awaCorr);
        }
        closeNvs(handle);
    }

    angleCorrRad = (float)(awaCorr) * AWA_CAL_SCALE;
}

void CalibrationStorage::ReadAwsCalibration(float &speedFactor) {
    int16_t awsCorr = DEFAULT_SPEED_FACTOR;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ( (err=nvs_get_i16(handle, NVS_KEY_AWS, &awsCorr)) != ESP_OK ){
            awsCorr = DEFAULT_SPEED_FACTOR;
            ESP_LOGI(TAG, "No AWS calibration found (%s) using default value %d", esp_err_to_name(err), awsCorr);
        }else{
            ESP_LOGI(TAG, "Read AWS calibration %d ", awsCorr);
        }
        closeNvs(handle);
    }

    speedFactor = (float)(awsCorr) * AWS_CAL_SCALE;
}

void CalibrationStorage::ReadSowCalibration(float &speedFactor) {
    int16_t speedCorr = DEFAULT_SPEED_FACTOR;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, NVS_KEY_SOW, &speedCorr)) != ESP_OK ){
            speedCorr = DEFAULT_SPEED_FACTOR;
            ESP_LOGI(TAG, "No SOW calibration found (%s) using default value %d", esp_err_to_name(err), speedCorr);
        }else{
            ESP_LOGI(TAG, "Read SOW calibration %d ", speedCorr);
        }
        closeNvs(handle);
    }

    speedFactor = (float)(speedCorr) * SOW_CAL_SCALE;
}

void CalibrationStorage::UpdateAwaCalibration(int16_t angleCorr, bool isRelative) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        int16_t currAwaCorr;
        // Read current value
        if (nvs_get_i16(handle, NVS_KEY_AWA, &currAwaCorr) != ESP_OK ){
            currAwaCorr = DEFAULT_ANGLE_CORR;
        }

        // Update with received delta
        int16_t newAwaCorr;
        if ( isRelative ){
            newAwaCorr = (int16_t)(currAwaCorr + angleCorr);
            ESP_LOGI(TAG, "AWA calibration %d + %d", currAwaCorr, angleCorr);
        }else{
            newAwaCorr = angleCorr;
        }

        // Store updated value
        esp_err_t err = nvs_set_i16(handle, NVS_KEY_AWA, newAwaCorr);
        ESP_LOGI(TAG, "AWA calibration %d  Stored %s", newAwaCorr, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "AWA calibration committed %s", err == ESP_OK ? "OK" : "Failed");
        closeNvs(handle);
    }
}

void CalibrationStorage::UpdateAwsCalibration(int16_t speedCorr, bool isRelative) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        int16_t currSpeedCorr;

        // Read current value
        if (nvs_get_i16(handle, NVS_KEY_AWS, &currSpeedCorr) != ESP_OK ){
            currSpeedCorr = DEFAULT_SPEED_FACTOR;
        }
        // Update with received delta
        int16_t newSpeedCorr;
        if ( isRelative ){
            newSpeedCorr = (int16_t)(((float)currSpeedCorr * AWS_CAL_SCALE * (float)speedCorr * AWS_CAL_SCALE) / AWS_CAL_SCALE);
            ESP_LOGI(TAG, "Relative AWS calibration %d * %d", currSpeedCorr, speedCorr);
        }else{
            newSpeedCorr = speedCorr;
        }

        esp_err_t err = nvs_set_i16(handle, NVS_KEY_AWS, newSpeedCorr);
        ESP_LOGI(TAG, "AWS calibration %d Stored %s", newSpeedCorr, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "AWS calibration committed %s", err == ESP_OK ? "OK" : "Failed");
        closeNvs(handle);
    }
}

void CalibrationStorage::UpdateSowCalibration(int16_t speedCorr, bool isRelative) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        int16_t currSpeedCorr;

        // Read current value
        if (nvs_get_i16(handle, NVS_KEY_SOW, &currSpeedCorr) != ESP_OK ){
            currSpeedCorr = DEFAULT_SPEED_FACTOR;
        }
        // Update with received delta
        int16_t newSpeedCorr;
        if ( isRelative ){
            newSpeedCorr = (int16_t)(((float)currSpeedCorr * SOW_CAL_SCALE * (float)speedCorr * SOW_CAL_SCALE) / SOW_CAL_SCALE);
            ESP_LOGI(TAG, "Relative SOW calibration %d * %d", currSpeedCorr, speedCorr);
        }else{
            newSpeedCorr = speedCorr;
        }

        esp_err_t err = nvs_set_i16(handle, NVS_KEY_SOW, newSpeedCorr);
        ESP_LOGI(TAG, "SOW calibration %d Stored %s", newSpeedCorr, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "SOW calibration committed %s", err == ESP_OK ? "OK" : "Failed");
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
