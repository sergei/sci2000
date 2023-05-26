#ifndef MHU2NMEA_N2KHANDLER_H
#define MHU2NMEA_N2KHANDLER_H

#include "freertos/FreeRTOS.h"
#include "ESP32N2kStream.h"
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <CustomPgnGroupFunctionHandler.h>

#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_34

#include "N2kMessages.h"
#include "NMEA2000_esp32_twai.h"
#include "LEDBlinker.h"

class N2KTwaiBusAlertListener: public TwaiBusAlertListener{
public:
    explicit N2KTwaiBusAlertListener(const xQueueHandle &evtQueue, LEDBlinker &ledBlinker);
    void onAlert(uint32_t alerts, bool isError) override ;
private:
    const xQueueHandle &m_evtQueue;
    LEDBlinker &m_ledBlinker;

};


static const int SCI_MFG_CODE = 2020;    // Our mfg code.
static const int SCI_INDUSTRY_CODE = 4;  // Marine industry
static const uint16_t SCI_IND_MFG_CODE = (SCI_INDUSTRY_CODE << 13) | (0x03 << 11) | SCI_MFG_CODE;

static const int PR_CODE = 133;
static const char *const MHU_MODEL_ID = "SCI USB->N2K";
static const char *const MFG_SW_VERSION = GIT_HASH;
static const char *const MFG_MODEL_VER = "1.0.0.0 (2023-06-01)";


static const int TWAI_TX_QUEUE_LEN = 20;


class N2KHandler : public SideTwaiBusInterface{

public:
    explicit N2KHandler(const xQueueHandle &evtQueue,  LEDBlinker &ledBlinker);
    void Start();
    bool addBusListener(TwaiBusListener *listener);
    void onSideIfcTwaiFrame(unsigned long id, unsigned char len, const unsigned char *buf) override;

    [[noreturn]] void N2KTask();

private:
    void Init();
    static void OnOpen();

    LEDBlinker &m_ledBlinker;
    N2KTwaiBusAlertListener m_busListener;


    ESP32N2kStream debugStream;

    TwaiBusListener *m_listeners[MAX_TWAI_LISTENERS] = {nullptr};
    int m_listenerCount = 0;
};


#endif //MHU2NMEA_N2KHANDLER_H
