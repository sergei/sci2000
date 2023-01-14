#include <esp_log.h>
#include <nvs_flash.h>
#include <N2kMessages.h>
#include "CalibrationStorage.h"

static const char *TAG = "mhu2nmea_CalibrationStorage";

void CalibrationStorage::ReadAwaCalibration(float &angleCorrRad) {
    int16_t awaCorr= DEFAULT_ANGLE_CORR_DEG;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, NVS_KEY_AWA, &awaCorr)) != ESP_OK ){
            awaCorr = DEFAULT_ANGLE_CORR_DEG;
            ESP_LOGI(TAG, "No AWA calibration found (%s) using default value %d", esp_err_to_name(err), awaCorr);
        }else{
            ESP_LOGI(TAG, "Read AWA calibration %d deg * %f", awaCorr, AWA_CAL_SCALE);
        }
        closeNvs(handle);
    }

    angleCorrRad = DegToRad((float)(awaCorr) * AWA_CAL_SCALE);
}

void CalibrationStorage::ReadAwsCalibration(float &speedFactor) {
    int16_t speedCorrPerc = DEFAULT_SPEED_FACTOR_PERC;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, NVS_KEY_AWS, &speedCorrPerc)) != ESP_OK ){
            speedCorrPerc = DEFAULT_SPEED_FACTOR_PERC;
            ESP_LOGI(TAG, "No AWS calibration found (%s) using default value %d", esp_err_to_name(err), speedCorrPerc);
        }else{
            ESP_LOGI(TAG, "Read AWS calibration %d ", speedCorrPerc);
        }
        closeNvs(handle);
    }

    speedFactor = 1.f + (float)(speedCorrPerc) * AWS_CAL_SCALE / 100.f;
}

void CalibrationStorage::ReadSowCalibration(float &speedFactor) {
    int16_t speedCorrPerc = DEFAULT_SPEED_FACTOR_PERC;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, NVS_KEY_SOW, &speedCorrPerc)) != ESP_OK ){
            speedCorrPerc = DEFAULT_SPEED_FACTOR_PERC;
            ESP_LOGI(TAG, "No SOW calibration found (%s) using default value %d", esp_err_to_name(err), speedCorrPerc);
        }else{
            ESP_LOGI(TAG, "Read SOW calibration %d ", speedCorrPerc);
        }
        closeNvs(handle);
    }

    speedFactor = 1.f + (float)(speedCorrPerc) * SOW_CAL_SCALE / 100.f;
}

void CalibrationStorage::StoreAwaCalibration(int16_t angleCorrDeg) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        esp_err_t err = nvs_set_i16(handle, NVS_KEY_AWA, angleCorrDeg);
        ESP_LOGI(TAG, "AWA calibration %d  Stored %s", angleCorrDeg, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "AWA calibration committed %s", err == ESP_OK ? "OK" : "Failed");
        closeNvs(handle);
    }
}

void CalibrationStorage::StoreAwsCalibration(int16_t speedCorrPerc) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        esp_err_t err = nvs_set_i16(handle, NVS_KEY_AWS, speedCorrPerc);
        ESP_LOGI(TAG, "AWS calibration %d Stored %s", speedCorrPerc, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "AWS calibration committed %s", err == ESP_OK ? "OK" : "Failed");
        closeNvs(handle);
    }
}

void CalibrationStorage::StoreSowCalibration(int16_t speedCorrPerc) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        esp_err_t err = nvs_set_i16(handle, NVS_KEY_SOW, speedCorrPerc);
        ESP_LOGI(TAG, "SOW calibration %d Stored %s", speedCorrPerc, err == ESP_OK ? "OK" : "Failed");
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
