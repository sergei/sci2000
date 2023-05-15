#ifndef IMU2NMEA_N2KBT_H
#define IMU2NMEA_N2KBT_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "N2kMessages.h"
#include "../NMEA2000_esp32_twai/NMEA2000_esp32_twai.h"
#include "SlipPacket.h"

class N2kBt : public TwaiBusListener, SlipListener, ByteOutputStream {

public:
    explicit N2kBt(SideTwaiBusInterface &twaiBusSender);
    // TWAI frame received from the bus
    void onTwaiFrameReceived(unsigned long id, unsigned char len, const unsigned char *buf) override;
    // TWAI frame transmitted to the bus
    void onTwaiFrameTransmit(unsigned long id, unsigned char len, const unsigned char *buf) override;

    void flush() override;

    void Start();

public:
    static void espBtGapCb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    void espBtSppCb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

    static N2kBt *instance;
private: // Constants
    const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
    const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
    const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;
    const char * SPP_SERVER_NAME = "N2K_SERVER";
    const char * EXAMPLE_DEVICE_NAME = "N2kSerialPort";
    const uint32_t INVALID_HANDLE = 0xFFFFFFFF;
    static const uint8_t MAX_CONNECTIONS = 20;
    static const uint16_t MAX_SPP_MTU = ESP_SPP_MAX_MTU;

private: // Methods
    void sendEncodedBytes(unsigned char *buf, unsigned char len) override;
    void onPacketReceived(const unsigned char *buf, unsigned char len) override;
    static char *bda2str(uint8_t *bda, char *str, size_t size);

private: // Fields
    SideTwaiBusInterface &twaiBusSender;
    SlipPacket slipPacket;
    uint32_t m_BtSppHandles[MAX_CONNECTIONS]{};
    bool m_bCongDetected = false;
    uint8_t m_ucSppBuffer[MAX_SPP_MTU]{};
    uint16_t m_usSppBufferLen = 0;
};


#endif //IMU2NMEA_N2KBT_H
