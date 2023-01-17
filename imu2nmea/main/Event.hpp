#ifndef IMU2NMEA_EVENT_HPP
#define IMU2NMEA_EVENT_HPP

#include "minmea.h"

enum EventSource {
    CAN_DRIVER_EVENT  // CAN bus event
    ,IMU              // IMU unit
    ,GPS_DATA_RMC         // GPS data ( 5 Hz rate)
    ,GPS_DATA_GGA         // GPS data ( 1 Hz rate)
};

struct Event {
    EventSource src;
    bool isValid;
    union  {
        float fValue;
        uint32_t uiValue;
        int32_t  iValue;
        struct {
            float hdg;
            float pitch;
            float roll;
            uint8_t sysCal;
            uint8_t gyroCal;
            uint8_t accCal;
            uint8_t magCal;
        }imu;
        struct {
            minmea_sentence_rmc rmc;
            minmea_sentence_gga gga;
        }gps;
    }u;
};

#endif //IMU2NMEA_EVENT_HPP
