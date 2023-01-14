#ifndef MHU2NMEA_CALIBRATIONSTORAGE_H
#define MHU2NMEA_CALIBRATIONSTORAGE_H

#include <nvs.h>

static const int16_t DEFAULT_SPEED_FACTOR_PERC = 0;
static const int16_t DEFAULT_ANGLE_CORR_DEG = 0;

static const char *const NVS_KEY_AWA = "cal_awa";
static const char *const NVS_KEY_AWS = "cal_aws";
static const char *const NVS_KEY_SOW = "cal_sow";

static const float AWA_CAL_SCALE = 0.01f;
static const float AWS_CAL_SCALE = 0.01f;
static const float SOW_CAL_SCALE = 0.01f;

class CalibrationStorage{
public:
    static void closeNvs(nvs_handle_t handle);
    static nvs_handle_t openNvs();

    // Reading already converted
    static void ReadAwaCalibration(float &angleCorrRad);
    static void ReadAwsCalibration(float &speedFactor);
    static void ReadSowCalibration(float &speedFactor);

    // Storing with the same scaling and units as received from N2K
    static void StoreAwaCalibration(int16_t angleCorrDeg);
    static void StoreAwsCalibration(int16_t speedCorrPerc);
    static void StoreSowCalibration(int16_t speedCorrPerc);
};



#endif //MHU2NMEA_CALIBRATIONSTORAGE_H
