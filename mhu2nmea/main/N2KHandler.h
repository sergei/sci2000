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

static const int SCI_MFG_CODE = 2020;    // Our mfg code.
static const int SCI_INDUSTRY_CODE = 4;  // Marine industry

/*
    Proprietary PGN 130900 to send/receive AWA and AWS calibration
    Field 1: MfgCode 11 bits
    Field 2: reserved 2 bits. Must be set all 1
    Field 3: Industry code 3 bits. Use Marine=4
    Field 4: AWAOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
    Field 5: AWSMultiplier, 2 bytes  0xfffe - restore default 0xFFFF - leave unchanged
 */
static const unsigned long MHU_CALIBRATION_PGN = 130900;  // Set/get MHU calibration
static const int MHU_CALIBRATION_RESTORE_DEFAULT = (int16_t) 0xfffe;

class N2KTwaiBusAlertListener: public TwaiBusAlertListener{
public:
    explicit N2KTwaiBusAlertListener(const xQueueHandle &evtQueue);
    void onAlert(uint32_t alerts, bool isError) override ;
private:
    const xQueueHandle &m_evtQueue;
};

class N2KHandler {

    /// Class to handle NMEA Group function commands sent by PGN 126208
    class MhuCalGroupFunctionHandler: public tN2kGroupFunctionHandler{
    public:
        MhuCalGroupFunctionHandler(N2KHandler &n2kHandler, tNMEA2000 *_pNMEA2000):
           tN2kGroupFunctionHandler(_pNMEA2000,MHU_CALIBRATION_PGN), m_n2kHandler(n2kHandler) {}
    protected:
        /// Network requested calibration values
        /// We reply with PGN 130900 containing these values
        bool HandleRequest(const tN2kMsg &N2kMsg,
                                   uint32_t TransmissionInterval,
                                   uint16_t TransmissionIntervalOffset,
                                   uint8_t  NumberOfParameterPairs,
                                   int iDev) override;
        /// Network wants to change the calibration
        /// This method decodes the comamnd and updates the calibration
        bool HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int iDev) override;
    private:
        N2KHandler &m_n2kHandler;
    };


public:
    explicit N2KHandler(const xQueueHandle &evtQueue);
    void StartTask();

    [[noreturn]] void N2KTask();

private:
    void Init();

    static void OnOpen();
    static bool SendCalValues();

    const xQueueHandle &m_evtQueue;
    MhuCalGroupFunctionHandler m_MhuCalGroupFunctionHandler;
    N2KTwaiBusAlertListener m_busListener;
    unsigned char uc_WindSeqId = 0;

    bool isAwaValid = false;
    float awaRad = 0;
    int64_t awaUpdateTime = 0;

    bool isAwsValid = false;
    float awsKts = 0;
    int64_t awsUpdateTime = 0;

    // Calibration values
    float m_awaCorrRad = 0;
    float m_awsFactor = 1;

    ESP32N2kStream debugStream;
    static tN2kSyncScheduler s_WindScheduler;

};


#endif //MHU2NMEA_N2KHANDLER_H
