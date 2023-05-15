#ifndef IMU2NMEA_N2KWIFI_H
#define IMU2NMEA_N2KWIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "N2kMessages.h"
#include "../NMEA2000_esp32_twai/NMEA2000_esp32_twai.h"
enum NetworkMsgType {
    CAN_FRAME
    ,WIFI_CONNECTED
    ,WIFI_DISCONNECTED
};

struct NetworkMsg{
    NetworkMsgType type=CAN_FRAME;
    unsigned long id = 0;
    unsigned char len = 0;
    unsigned char pdu[8] ={};
};

struct SsidPassList{
    const char* ssid;
    const char* password;
};
const int UDP_RX_PORT = 2023;
const int UDP_TX_PORT = 2024;
const int DEFAULT_SCAN_LIST_SIZE = 256;

class N2kWifi : public TwaiBusListener {
public:
    N2kWifi(SideTwaiBusInterface &twaiBusSender);
    void Start();
    [[noreturn]] void TransmitFrameTask();
    [[noreturn]] void ReceiveFrameTask();

    void WifiEventHandler(int32_t event_id, void *event_data);
    void onTwaiFrameReceived(unsigned long id, unsigned char len, const unsigned char *buf) override;
    void onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) override;
    void flush() override;

private:
    static bool CheckScanResults(char *ssid, char *password);
    void StartWifi();
    void StartServer();
    void StopServer();
    void ListenForUdpInput();

private:
    SideTwaiBusInterface &twaiBusSender;
    xQueueHandle txFrameQueue;
    xQueueHandle rxFrameQueue;
    bool volatile isWifiConnected = false;

    void BroadcastCanFramesOverUdp();

    static int CreateBoundUdpSocket();
};


#endif //IMU2NMEA_N2KWIFI_H
