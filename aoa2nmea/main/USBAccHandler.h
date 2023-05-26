#ifndef IMU2NMEA_GPSHANDLER_H
#define IMU2NMEA_GPSHANDLER_H


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <hal/uart_types.h>
#include <driver/uart.h>
#include <NMEA2000_esp32_twai.h>


class USBAccHandler :  public TwaiBusListener {
public:
    USBAccHandler(SideTwaiBusInterface &twaiBusSender, int tx_io_num, int rx_io_num, uart_port_t uart_num);
    void Start();
    [[noreturn]] void Task();
public: // TwaiBusListener methods
    // TWAI frame received from the bus
    void onTwaiFrameReceived(unsigned long id, unsigned char len, const unsigned char *buf) override;
    // TWAI frame transmitted to the bus
    void onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) override;
    // Flush any buffered data
    void flush() override;
private:
    SideTwaiBusInterface &twaiBusSender;
    const int tx_io_num;
    const int rx_io_num;
    const uart_port_t uart_num;
    QueueHandle_t m_uartEventQueue = nullptr;
    const int uart_buffer_size = 4 * 1024;

};


#endif //IMU2NMEA_GPSHANDLER_H
