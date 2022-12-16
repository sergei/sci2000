#ifndef MHU2NMEA_EVENT_HPP
#define MHU2NMEA_EVENT_HPP

enum EventSource {
    AWA
    ,AWS
    ,CAN_DRIVER_EVENT
    ,CALIBRATION_UPDATED
};

struct Event {
    EventSource src;
    bool isValid;
    union {
        float fValue;
        uint32_t uiValue;
        int32_t  iValue;
    }u;
};

#endif //MHU2NMEA_EVENT_HPP
