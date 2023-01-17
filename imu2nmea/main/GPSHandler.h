#ifndef IMU2NMEA_GPSHANDLER_H
#define IMU2NMEA_GPSHANDLER_H


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class GPSHandler {
public:
    GPSHandler(const xQueueHandle &eventQueue, int tx_io_num, int rx_io_num);
    void Start();
    [[noreturn]] void Task();
private:
    const xQueueHandle &eventQueue;
    int tx_io_num;
    int rx_io_num;
};


#endif //IMU2NMEA_GPSHANDLER_H
