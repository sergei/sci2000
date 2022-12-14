#ifndef MHU2NMEA_N2KHANDLER_H
#define MHU2NMEA_N2KHANDLER_H

#include "freertos/FreeRTOS.h"
#include "ESP32N2kStream.h"
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_34

#include "NMEA2000_esp32.h"
#include "N2kMessages.h"
#include "NMEA2000_esp32_twai.h"

class N2KTwaiBusAlertListener: public TwaiBusAlertListener{
public:
    N2KTwaiBusAlertListener(const xQueueHandle &evtQueue);
    void onAlert(uint32_t alerts, bool isError) override ;
private:
    const xQueueHandle &m_evtQueue;
};

class N2KHandler {
public:
    explicit N2KHandler(const xQueueHandle &evtQueue);
    void StartTask();

    [[noreturn]] void N2KTask();

private:
    void Init();
    static void OnOpen();
    const xQueueHandle &m_evtQueue;
    N2KTwaiBusAlertListener m_busListener;
    unsigned char uc_WindSeqId = 0;

    bool isAwaValid = false;
    float awaRad = 0;
    int64_t awaUpdateTime = 0;

    bool isAwsValid = false;
    float awsKts = 0;
    int64_t awsUpdateTime = 0;

    ESP32N2kStream debugStream;
    static tN2kSyncScheduler s_WindScheduler;

};


#endif //MHU2NMEA_N2KHANDLER_H
