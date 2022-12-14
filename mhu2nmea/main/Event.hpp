#ifndef MHU2NMEA_EVENT_HPP
#define MHU2NMEA_EVENT_HPP

enum EventSource {
    AWA,
    AWS,
    CAN_DRIVER_EVENT
};

struct Event {
    EventSource src;
    bool isValid;
    union {
        float fValue;
        uint32_t uiValue;
    }u;
};

#endif //MHU2NMEA_EVENT_HPP
