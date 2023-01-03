#ifndef MHU2NMEA_ESP32N2KSTREAM_H
#define MHU2NMEA_ESP32N2KSTREAM_H

#include "N2kStream.h"

#define HEX 16

class ESP32N2kStream : public N2kStream{
    // Returns first byte if incoming data, or -1 on no available data.
    int read() override;
    int peek() override;

    // Write data to stream.
    size_t write(const uint8_t* data, size_t size) override;
private:
    char buff[2048];
    int  len = 0;
public:
    void println();
    void println(long l);
    void println(char *l);
    void println(const char* format, ...);
};


#endif //MHU2NMEA_ESP32N2KSTREAM_H
