#ifndef MHU2NMEA_N2KHANDLER_H
#define MHU2NMEA_N2KHANDLER_H

#include "freertos/FreeRTOS.h"
#include "ESP32N2kStream.h"
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_34

#include "N2kMessages.h"
#include "NMEA2000_esp32_twai.h"

class N2KTwaiBusAlertListener: public TwaiBusAlertListener{
public:
    explicit N2KTwaiBusAlertListener(const xQueueHandle &evtQueue);
    void onAlert(uint32_t alerts, bool isError) override ;
private:
    const xQueueHandle &m_evtQueue;
};


static const int SCI_MFG_CODE = 2020;    // Our mfg code.
static const int SCI_INDUSTRY_CODE = 4;  // Marine industry
static const int IMU_PR_CODE = 132;
static const char *const MHU_MODEL_ID = "SCI IMU->N2K";
static const char *const MFG_SW_VERSION = "1.0.0.0 (2022-12-07)";
static const char *const MFG_MODEL_VER = "1.0.0.0 (2022-12-07)";

static const unsigned long SPEED_CALIBRATION_PGN = 130901;  // Set/get speed calibration

enum {
    DEV_IMU,
    DEV_NUM
};


static const int IMU_TOUT = 10 * 1000000;

class N2KHandler {

public:
    explicit N2KHandler(const xQueueHandle &evtQueue);
    void Start();

    [[noreturn]] void N2KTask();

private:
    void Init();
    static void OnOpen();

    const xQueueHandle &m_evtQueue;
    N2KTwaiBusAlertListener m_busListener;

    unsigned char uc_ImuSeqId = 0;

    bool isImuValid = false;
    int64_t imuUpdateTime = 0;
    float hdg=0;
    float pitch=0;
    float roll=0;

    ESP32N2kStream debugStream;
    static tN2kSyncScheduler s_ImuScheduler;
};


#endif //MHU2NMEA_N2KHANDLER_H
