#ifndef MHU2NMEA_EVENT_HPP
#define MHU2NMEA_EVENT_HPP

enum EventSource {
    AWA_POLL,
    AWS_INR
};

struct Event {
    EventSource src;
    union {
        struct {
            bool isValid;
            float value;
        }awa;
        struct {
          int unit;           // the PCNT unit that originated an interrupt
          uint32_t status;    // information on the event type that caused the interrupt
          int64_t elapsed_us; // Time since last event
        }ctr;
    }u;
};

#endif //MHU2NMEA_EVENT_HPP
