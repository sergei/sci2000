#ifndef MHU2NMEA_CALIBRATIONSTORAGE_H
#define MHU2NMEA_CALIBRATIONSTORAGE_H

#include <nvs.h>

static const int16_t DEFAULT_AWS_CORR = 1000;
static const int16_t DEFAULT_AWA_CORR = 0;

static const char *const NVS_KEY_AWA = "cal_awa";
static const char *const NVS_KEY_AWS = "cal_aws";


static const float AWA_CAL_SCALE = 0.001f;
static const float AWS_CAL_SCALE = 0.001f;

class CalibrationStorage{
public:
    static void closeNvs(nvs_handle_t handle);
    static nvs_handle_t openNvs();
    static void ReadCalibration(float &awaCorrRad, float &awsFactor);
    static void UpdateAwaCalibration(int16_t awaCorr, bool isRelative = false);
    static void UpdateAwsCalibration(int16_t awsCorr, bool isRelative = false);
};



#endif //MHU2NMEA_CALIBRATIONSTORAGE_H
