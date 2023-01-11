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
    explicit N2KTwaiBusAlertListener(const xQueueHandle &evtQueue);
    void onAlert(uint32_t alerts, bool isError) override ;
private:
    const xQueueHandle &m_evtQueue;
};


static const int SCI_MFG_CODE = 2020;    // Our mfg code.
static const int SCI_INDUSTRY_CODE = 4;  // Marine industry
static const int MHU_PR_CODE = 130;
static const char *const MHU_MODEL_ID = "SCI MHU->N2K";
static const int SPEED_PR_CODE = 131;
static const char *const SPEED_MODEL_ID = "SCI Speed->N2K";
static const char *const MFG_SW_VERSION = "1.0.0.0 (2022-12-07)";
static const char *const MFG_MODEL_VER = "1.0.0.0 (2022-12-07)";

static const int CALIBRATION_RESTORE_DEFAULT = (int16_t) 0xfffe;
static const unsigned long MHU_CALIBRATION_PGN = 130900;  // Set/get MHU calibration
/*
    Proprietary PGN 130900 the AWA and AWS calibration
    Field 1: MfgCode 11 bits
    Field 2: reserved 2 bits. Must be set all 1
    Field 3: Industry code 3 bits. Use Marine=4
    Field 4: AWAOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
    Field 5: AWSMultiplier, 2 bytes  0xfffe - restore default 0xFFFF - leave unchanged
 */

static const unsigned long SPEED_CALIBRATION_PGN = 130901;  // Set/get speed calibration

/*
    Proprietary PGN 130901 the boat speed calibration
    Field 1: MfgCode 11 bits
    Field 2: reserved 2 bits. Must be set all 1
    Field 3: Industry code 3 bits. Use Marine=4
    Field 4: SOWMultiplier, 2 bytes  0xfffe - restore default 0xFFFF - leave unchanged
 */

enum {
    DEV_MHU,
    DEV_SPEED,

    DEV_NUM
};


static const int AWS_TOUT = 10 * 1000000;
static const int AWA_TOUT = 10 * 1000000;
static const int SOW_TOUT = 10 * 1000000;

class N2KHandler {

    /// Class to handle NMEA Group function commands sent by PGN 126208  for PGN 130900 to send/receive AWA and AWS calibration
    class MhuCalGroupFunctionHandler: public CustomPgnGroupFunctionHandler{
    public:
        MhuCalGroupFunctionHandler(N2KHandler &n2kHandler, tNMEA2000 *_pNMEA2000):
                CustomPgnGroupFunctionHandler(_pNMEA2000,MHU_CALIBRATION_PGN,
                                              SCI_MFG_CODE, SCI_MFG_CODE), m_n2kHandler(n2kHandler) {}
    protected:
        /// Network requested calibration values
        /// We reply with PGN 130900 containing these values
        bool ProcessRequest(const tN2kMsg &N2kMsg,
                                    uint32_t TransmissionInterval,
                                    uint16_t TransmissionIntervalOffset,
                                    uint8_t  NumberOfParameterPairs,
                                    int Index,
                                    int iDev) override;
        /// Network wants to change the calibration
        /// This method decodes the command and updates the calibration
        bool ProcessCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int Index, int iDev) override;
    private:
        N2KHandler &m_n2kHandler;
    };

    /// Class to handle NMEA Group function commands sent by PGN 126208  for PGN 130901 to send/receive SOW calibration
    class BoatSpeedCalGroupFunctionHandler: public CustomPgnGroupFunctionHandler{
    public:
        BoatSpeedCalGroupFunctionHandler(N2KHandler &n2kHandler, tNMEA2000 *_pNMEA2000):
                CustomPgnGroupFunctionHandler(_pNMEA2000,SPEED_CALIBRATION_PGN,
                                              SCI_MFG_CODE, SCI_MFG_CODE), m_n2kHandler(n2kHandler) {}
    protected:
        /// Network requested calibration values
        /// We reply with PGN 130900 containing these values
        bool ProcessRequest(const tN2kMsg &N2kMsg,
                                    uint32_t TransmissionInterval,
                                    uint16_t TransmissionIntervalOffset,
                                    uint8_t  NumberOfParameterPairs,
                                    int Index,
                                    int iDev) override;
        /// Network wants to change the calibration
        /// This method decodes the command and updates the calibration
        bool ProcessCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int Index, int iDev) override;
    private:
        N2KHandler &m_n2kHandler;
    };

public:
    explicit N2KHandler(const xQueueHandle &evtQueue, LEDBlinker &ledBlinker);
    void StartTask();

    [[noreturn]] void N2KTask();

private:
    void Init();
    static void OnOpen();

    /// Send PGN GET_MHU_CALIBRATION_PGN
    static bool SendMhuCalValues();
    /// Send PGN SPEED_CALIBRATION_PGN
    static bool SendBoatSpeedCalValues();

    const xQueueHandle &m_evtQueue;
    LEDBlinker &m_ledBlinker;
    MhuCalGroupFunctionHandler m_MhuCalGroupFunctionHandler;
    BoatSpeedCalGroupFunctionHandler m_BoatSpeedCalGroupFunctionHandler;
    N2KTwaiBusAlertListener m_busListener;
    unsigned char uc_WindSeqId = 0;
    unsigned char uc_BoatSpeedSeqId = 0;

    bool isAwaValid = false;
    float awaRad = 0;
    int64_t awaUpdateTime = 0;

    bool isAwsValid = false;
    float awsKts = N2kFloatNA;
    int64_t awsUpdateTime = 0;

    bool isSowValid = false;
    float sowKts = N2kFloatNA;
    int64_t sowUpdateTime = 0;

    // Calibration values
    float m_awaCorrRad = 0;
    float m_awsFactor = 1;
    float m_sowFactor = 1;

    ESP32N2kStream debugStream;
    static tN2kSyncScheduler s_WindScheduler;
    static tN2kSyncScheduler s_WaterScheduler;
};


#endif //MHU2NMEA_N2KHANDLER_H
