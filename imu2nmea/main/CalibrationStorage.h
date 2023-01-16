#ifndef MHU2NMEA_CALIBRATIONSTORAGE_H
#define MHU2NMEA_CALIBRATIONSTORAGE_H

#include <nvs.h>

static const int16_t DEFAULT_ANGLE_CORR = 0;
static const float ANGLE_CAL_SCALE = 0.01f;

static const char *const NVS_KEY_HDG = "HDG";
static const char *const NVS_KEY_ROLL = "ROLL";
static const char *const NVS_KEY_PITCH = "PITCH";

class CalibrationStorage{
public:
    static void ReadHeadingCalibration(float &angleCorrRad);
    static void ReadPitchCalibration(float &angleCorrRad);
    static void ReadRollCalibration(float &angleCorrRad);

    static void UpdateHeadingCalibration(int16_t angleCorr);
    static void UpdatePitchCalibration(int16_t angleCorr);
    static void UpdateRollCalibration(int16_t angleCorr);
private:
    static void closeNvs(nvs_handle_t handle);
    static nvs_handle_t openNvs();

    static void readAngleCorr(const char *key, float &angleCorrRad);
    static void updateAngleCorrection(const char *key, int16_t angleCorr);
};



#endif //MHU2NMEA_CALIBRATIONSTORAGE_H
