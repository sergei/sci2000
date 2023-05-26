#ifndef IMU2NMEA_EVENT_HPP
#define IMU2NMEA_EVENT_HPP

enum EventSource {
    CAN_DRIVER_EVENT  // CAN bus event
};

struct Event {
    EventSource src;
    bool isValid;
    union  {
        float fValue;
        uint32_t uiValue;
        int32_t  iValue;
    }u;
};

#endif //IMU2NMEA_EVENT_HPP
