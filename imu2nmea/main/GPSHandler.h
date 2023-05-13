#ifndef IMU2NMEA_GPSHANDLER_H
#define IMU2NMEA_GPSHANDLER_H


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <hal/uart_types.h>
#include <driver/uart.h>
#include "UbxParser.h"

class GpsParser : public UbxParser::Listener {
public:
    explicit GpsParser(QueueHandle_t const &systemEventQueue, UbxParser &ubxParser);
    void ProcessInputBytes(const uint8_t *buff, size_t size);
    void onUbxMsg(uint8_t cls, uint8_t type, UBX_message_t msg) override;

private:
    int  m_lineBufferIdx = 0;
    uint8_t m_lineBuffer[256]={};

    void ParseNmea0183Line(const char *lineToParse);
    static void printHex(const uint8_t *buff, size_t size) ;
    const xQueueHandle &systemEventQueue;
    UbxParser &m_ubxParser;
};


class GPSHandler : public UbxParser::Writer{
public:
    GPSHandler(const xQueueHandle &eventQueue, int tx_io_num, int rx_io_num, uart_port_t uart_num);
    void Start();
    [[noreturn]] void Task();
    void writeToUbx(const uint8_t *b, size_t len) override;
private:
    GpsParser m_gpsParser;
    const int tx_io_num;
    const int rx_io_num;
    const uart_port_t uart_num;
    UbxParser m_ubxParser;
    QueueHandle_t m_uartEventQueue = nullptr;
    const int uart_buffer_size = 4 * 1024;

    void initUbx();
};


#endif //IMU2NMEA_GPSHANDLER_H
