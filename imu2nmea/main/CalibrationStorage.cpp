#include <esp_log.h>
#include <nvs_flash.h>
#include <N2kMessages.h>
#include "CalibrationStorage.h"

static const char *TAG = "imu2nmea_CalibrationStorage";

void CalibrationStorage::ReadHeadingCalibration(float &angleCorrRad) {
    readAngleCorr(NVS_KEY_HDG, angleCorrRad);
}
void CalibrationStorage::UpdateHeadingCalibration(int16_t angleCorr) {
    updateAngleCorrection(NVS_KEY_HDG, angleCorr);
}
void CalibrationStorage::ReadRollCalibration(float &angleCorrRad) {
    readAngleCorr(NVS_KEY_ROLL, angleCorrRad);
}

void CalibrationStorage::UpdateRollCalibration(int16_t angleCorr) {
    updateAngleCorrection(NVS_KEY_ROLL, angleCorr);
}

void CalibrationStorage::ReadPitchCalibration(float &angleCorrRad) {
    readAngleCorr(NVS_KEY_PITCH, angleCorrRad);
}

void CalibrationStorage::UpdatePitchCalibration(int16_t angleCorr) {
    updateAngleCorrection(NVS_KEY_PITCH, angleCorr);
}

void CalibrationStorage::readAngleCorr(const char *key, float &angleCorrRad) {
    int16_t corr= DEFAULT_ANGLE_CORR;

    nvs_handle_t handle = openNvs();
    if (handle){
        esp_err_t err;
        if ((err=nvs_get_i16(handle, key, &corr)) != ESP_OK ){
            corr = DEFAULT_ANGLE_CORR;
            ESP_LOGI(TAG, "No %s calibration found (%s) using default value %d", key, esp_err_to_name(err), corr);
        }else{
            ESP_LOGI(TAG, "Read %s calibration %d ", key, corr);
        }
        closeNvs(handle);
    }

    angleCorrRad = (float)DegToRad((float)(corr) * ANGLE_CAL_SCALE);
}

void CalibrationStorage::updateAngleCorrection(const char *key, int16_t angleCorr) {
    nvs_handle_t handle = openNvs();
    if (handle) {
        // Store updated value
        esp_err_t err = nvs_set_i16(handle, key, angleCorr);
        ESP_LOGI(TAG, "%s calibration %d  Stored %s", key, angleCorr, err == ESP_OK ? "OK" : "Failed");
        err = nvs_commit(handle);
        ESP_LOGI(TAG, "%s calibration committed %s", key, err == ESP_OK ? "OK" : "Failed");
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

