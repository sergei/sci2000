#ifndef IMU2NMEA_IMUCALINTERFACE_H
#define IMU2NMEA_IMUCALINTERFACE_H
class IMUCalInterface {
public:
    virtual void StoreCalibration() = 0;
    virtual void EraseCalibration() = 0;
};
#endif //IMU2NMEA_IMUCALINTERFACE_H
