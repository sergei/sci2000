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
static const int IMU_PR_CODE = 132;
static const char *const MHU_MODEL_ID = "SCI IMU->N2K";
static const char *const MFG_SW_VERSION = "1.0.0.0 (2022-12-07)";
static const char *const MFG_MODEL_VER = "1.0.0.0 (2022-12-07)";
enum {
    DEV_IMU,
    DEV_NUM
};

static const int CALIBRATION_RESTORE_DEFAULT = (int16_t) 0xfffe;
static const unsigned long IMU_CALIBRATION_PGN = 130902;  // IMU calibration
/*
    Proprietary PGN 130902 IMU calibration
    Field 1: MfgCode 11 bits
    Field 2: reserved 2 bits. Must be set all 1
    Field 3: Industry code 3 bits. Use Marine=4
    Field 4: HeadingGOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
    Field 5: PitchOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
    Field 6: RollOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
 */


static const int IMU_TOUT = 10 * 1000000;

class N2KHandler {

    /// Class to handle NMEA Group function commands sent by PGN 126208  for PGN 130902 to send/receive IMU calibration
    class ImuCalGroupFunctionHandler: public CustomPgnGroupFunctionHandler{
    public:
        ImuCalGroupFunctionHandler(N2KHandler &n2kHandler, tNMEA2000 *_pNMEA2000):
                CustomPgnGroupFunctionHandler(_pNMEA2000,IMU_CALIBRATION_PGN,
                                              SCI_MFG_CODE, SCI_MFG_CODE), m_n2kHandler(n2kHandler) {}
    protected:
        /// Network requested calibration values
        /// We reply with PGN 130902 containing these values
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
    explicit N2KHandler(const xQueueHandle &evtQueue,  LEDBlinker &ledBlinker);
    void Start();

    [[noreturn]] void N2KTask();

private:
    void Init();
    static void OnOpen();

    const xQueueHandle &m_evtQueue;
    LEDBlinker &m_ledBlinker;
    N2KTwaiBusAlertListener m_busListener;

    unsigned char uc_ImuSeqId = 0;

    bool isImuValid = false;
    int64_t imuUpdateTime = 0;
    float hdg=0;
    float pitch=0;
    float roll=0;

    ESP32N2kStream debugStream;
    static tN2kSyncScheduler s_ImuScheduler;

    static bool SendImuCalValues();

    // Calibration values
    float &m_hdgCorrRad = 0;
    float &m_pitchCorrRad = 0;
    float &m_rollCorrRad = 0;
};


#endif //MHU2NMEA_N2KHANDLER_H
