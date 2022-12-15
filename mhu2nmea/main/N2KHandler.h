#ifndef MHU2NMEA_N2KHANDLER_H
#define MHU2NMEA_N2KHANDLER_H

#include "freertos/FreeRTOS.h"
#include "ESP32N2kStream.h"
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <nvs.h>

#define ESP32_CAN_TX_PIN GPIO_NUM_32
#define ESP32_CAN_RX_PIN GPIO_NUM_34

static const int16_t DEFAULT_AWS_CORR = 1000;
static const int16_t DEFAULT_AWA_CORR = 0;

static const char *const NVS_KEY_AWA = "awa";
static const char *const NVS_KEY_AWS = "aws";

#include "N2kMessages.h"
#include "NMEA2000_esp32_twai.h"

const unsigned long SET_MHU_CALIBRATION_PGN = 130900;
const unsigned long GET_MHU_CALIBRATION_PGN = 130900;

enum MHU_CALIBR_DEST {
    MHU_CALIBR_DEST_AWA = 0,
    MHU_CALIBR_DEST_AWS = 1,
};

class N2KTwaiBusAlertListener: public TwaiBusAlertListener{
public:
    explicit N2KTwaiBusAlertListener(const xQueueHandle &evtQueue);
    void onAlert(uint32_t alerts, bool isError) override ;
private:
    const xQueueHandle &m_evtQueue;
};

class CalibrationMessageHandler: public tNMEA2000::tMsgHandler {
public:
    explicit CalibrationMessageHandler(const xQueueHandle &evtQueue, unsigned long _PGN, tNMEA2000 *_pNMEA2000);
    void HandleMsg(const tN2kMsg &N2kMsg) override;
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

    static nvs_handle_t openNvs();
    static void closeNvs(nvs_handle_t handle);
    void ReadCalibration();
    static void StoreAwaCalibration(int16_t awaCorr) ;
    static void StoreAwsCalibration(int16_t awsCorr) ;

    static void OnOpen();

    const xQueueHandle &m_evtQueue;
    CalibrationMessageHandler m_calMsgHandler;
    N2KTwaiBusAlertListener m_busListener;
    unsigned char uc_WindSeqId = 0;

    bool isAwaValid = false;
    float awaRad = 0;
    int64_t awaUpdateTime = 0;

    bool isAwsValid = false;
    float awsKts = 0;
    int64_t awsUpdateTime = 0;

    // Calibration values
    int16_t m_awaCorr = DEFAULT_AWA_CORR;     // Add this value to get corrected AWA (tenth of degree)
    int16_t m_awsCorr = DEFAULT_AWS_CORR;  // Multiply by this value to get corrected AWS (pro mile)

    ESP32N2kStream debugStream;
    static tN2kSyncScheduler s_WindScheduler;

};


#endif //MHU2NMEA_N2KHANDLER_H
