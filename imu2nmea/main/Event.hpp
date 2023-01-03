#ifndef IMU2NMEA_EVENT_HPP
#define IMU2NMEA_EVENT_HPP

enum EventSource {
    CAN_DRIVER_EVENT  // CAN bus event
    ,IMU              // IMU unit
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
    }u;
};

#endif //IMU2NMEA_EVENT_HPP
