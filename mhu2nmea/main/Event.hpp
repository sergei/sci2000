#ifndef MHU2NMEA_EVENT_HPP
#define MHU2NMEA_EVENT_HPP

enum EventSource {
    AWA
    ,AWS
    ,CAN_DRIVER_EVENT
    ,MHU_AWA_CALIB_RECEIVED
    ,MHU_AWS_CALIB_RECEIVED
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
