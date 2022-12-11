#ifndef MHU2NMEA_ESP32N2KSTREAM_H
#define MHU2NMEA_ESP32N2KSTREAM_H

#include "N2kStream.h"

class ESP32N2kStream : public N2kStream{
    // Returns first byte if incoming data, or -1 on no available data.
    int read() override;
    int peek() override;

    // Write data to stream.
    size_t write(const uint8_t* data, size_t size) override;
private:
    char buff[2048];
    int  len = 0;
};


#endif //MHU2NMEA_ESP32N2KSTREAM_H
