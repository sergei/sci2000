#ifndef MHU2NMEA_EVENT_HPP
#define MHU2NMEA_EVENT_HPP

enum EventSource {
    AWA  // Apparent wind angle
    ,AWS // Apparent wind speed
    ,SOW // Speed over water
    ,CAN_DRIVER_EVENT
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
