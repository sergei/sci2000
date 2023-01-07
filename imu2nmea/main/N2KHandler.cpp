#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"
#include "CalibrationStorage.h"

NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN, TWAI_MODE_NORMAL);

static const unsigned long TX_PGNS_IMU[] PROGMEM={127250, // Vessel Heading
                                                  127257, // Attitude
                                                  0};

static const unsigned long RX_PGNS_IMU[] PROGMEM={
                                              0};

static const char *TAG = "imu2nmea_N2KHandler";

tN2kSyncScheduler N2KHandler::s_ImuScheduler(false, 200, 0);

N2KHandler::N2KHandler(const xQueueHandle &evtQueue, LEDBlinker &ledBlinker)
    :m_evtQueue(evtQueue)
    ,m_ledBlinker(ledBlinker)
    ,m_busListener(evtQueue)
{
}

void N2KHandler::Init() {

    NMEA2000.SetDeviceCount(DEV_NUM); // Enable multi device support for 2 devices

    esp_log_level_set("NMEA2000_esp32_twai", ESP_LOG_DEBUG); // enable DEBUG logs from ESP32_TWAI layer
    NMEA2000.SetForwardStream(&debugStream);         // PC output on idf monitor
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text
    NMEA2000.EnableForward(true);                       // Disable all msg forwarding to USB (=Serial)

    NMEA2000.setBusEventListener(&m_busListener);

    NMEA2000.SetN2kCANMsgBufSize(8);
    NMEA2000.SetN2kCANReceiveFrameBufSize(100);

    uint8_t chipid[8];
    esp_efuse_mac_get_default(chipid);
    uint32_t SerialNumberImu = ((uint32_t)chipid[0]) << 24 | ((uint32_t)chipid[1]) << 16 | ((uint32_t)chipid[2]) << 8 | ((uint32_t)chipid[3]);
    char SnoStrImu[33];
    snprintf(SnoStrImu, 32, "%lu", (long unsigned int)SerialNumberImu);

    NMEA2000.SetProductInformation(SnoStrImu, // Manufacturer's Model serial code
                                   IMU_PR_CODE,  // Manufacturer's product code
                                   MHU_MODEL_ID,     // Manufacturer's Model ID
                                   MFG_SW_VERSION,   // Manufacturer's Software version code
                                   MFG_MODEL_VER,// Manufacturer's Model version
                                   0xff,       // Load equivalency Default=1. x * 50 mA
                                   0xffff,         // N2K version Default=2101
                                   0xff,       // Certification level Default=1
                                   DEV_IMU
    );

    // Set device information
    NMEA2000.SetDeviceInformation(SerialNumberImu, // Unique number.
                                  140,             // Ownship AttitudeDevices that report heading, pitch, roll, yaw, angular rates
                                    60,              // Device class    = Navigation Equipment that provide information related to the passage of the vessel and potental obstructions/hazards {Formerly NavigationSystems}
                               SCI_MFG_CODE ,
                                  SCI_INDUSTRY_CODE,
                                  DEV_IMU
    );

    NMEA2000.SetConfigurationInformation("https://github.com/sergei/sci2000");

    NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly);
    NMEA2000.ExtendTransmitMessages(TX_PGNS_IMU, DEV_IMU);

    NMEA2000.ExtendReceiveMessages(RX_PGNS_IMU, DEV_IMU);

    NMEA2000.SetOnOpen(OnOpen);
}

static void n2k_task( void *me ) {
    ((N2KHandler *)me)->N2KTask();
}

void N2KHandler::Start() {
    xTaskCreate(
            n2k_task,         /* Function that implements the task. */
            "N2KTask",            /* Text name for the task. */
            16 * 1024,        /* Stack size in words, not bytes. */
            ( void * ) this,  /* Parameter passed into the task. */
            tskIDLE_PRIORITY + 1, /* Priority at which the task is created. */
            nullptr );        /* Used to pass out the created task's handle. */
}

[[noreturn]] void N2KHandler::N2KTask() {
    ESP_LOGI(TAG, "Starting N2K task ");
    Init();

    CalibrationStorage::ReadHeadingCalibration(m_hdgCorrRad);
    CalibrationStorage::ReadPitchCalibration(m_pitchCorrRad);
    CalibrationStorage::ReadRollCalibration(m_rollCorrRad);

    for( ;; ) {
        Event evt{};
        portBASE_TYPE res = xQueueReceive(m_evtQueue, &evt, 1);

        // Check if new  data is available
        if (res == pdTRUE) {
            switch (evt.src) {
                case IMU:
                    hdg = evt.u.imu.hdg + RadToDeg(m_hdgCorrRad);
                    pitch = evt.u.imu.pitch + RadToDeg(m_pitchCorrRad);
                    roll = evt.u.imu.roll + RadToDeg(m_rollCorrRad);
                    isImuValid = evt.isValid;
                    if( isImuValid ){
                        imuUpdateTime = esp_timer_get_time();
                    }
                    break;
                case CAN_DRIVER_EVENT:
                    break;
            }
        }

        int64_t now = esp_timer_get_time();
        // Check age and invalidate if it's too old
        if ( now - imuUpdateTime > IMU_TOUT){
            isImuValid = false;
        }

        // Check if it's time to send the messages
        if ( s_ImuScheduler.IsTime() ) {
            s_ImuScheduler.UpdateNextTime();

            tN2kMsg N2kMsg;
            double localHdgRad = isImuValid ? DegToRad(hdg) : N2kDoubleNA;

            SetN2kMagneticHeading(N2kMsg, this->uc_ImuSeqId, localHdgRad);
            bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
            m_ledBlinker.SetBusState(sentOk);
            ESP_LOGD(TAG, "SetN2kMagneticHeading HDG=%.1f  %s", RadToDeg(localHdgRad), sentOk ? "OK" : "Failed");

            double localRollRad = isImuValid ? DegToRad(roll) : N2kDoubleNA;
            double localPitchRad = isImuValid ? DegToRad(pitch) : N2kDoubleNA;

            SetN2kAttitude(N2kMsg, this->uc_ImuSeqId++, localHdgRad,localPitchRad, localRollRad);
            sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
            ESP_LOGD(TAG, "SetN2kMagneticHeading HDG=%.1f PITCH=%.0f ROLL=%.0f %s", RadToDeg(localHdgRad), RadToDeg(localPitchRad), RadToDeg(localRollRad), sentOk ? "OK" : "Failed");
            m_ledBlinker.SetBusState(sentOk);
        }

        // crank NMEA2000 state machine
        NMEA2000.ParseMessages();
    }
}

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
void N2KHandler::OnOpen() {
    // Start schedulers now.
    s_ImuScheduler.UpdateNextTime();
}

N2KTwaiBusAlertListener::N2KTwaiBusAlertListener(QueueHandle_t const &evtQueue)
        :m_evtQueue(evtQueue)
{
}

void N2KTwaiBusAlertListener::onAlert(uint32_t alerts, bool isError) {
    if ( isError ){
        ESP_LOGE(TAG, "CAN driver error");
    }else{
        // CAN bus available, crank the N2K FSM
        Event evt = {
                .src = CAN_DRIVER_EVENT,
                .isValid = true,
                .u= {.uiValue = alerts}

        };
        xQueueSend(m_evtQueue, &evt, 0);
    }
}

bool N2KHandler::SendImuCalValues() {
    // Get calibration values
    float hdgCorrRad, rollCorrRad, pitchCorrRad;
    CalibrationStorage::ReadHeadingCalibration(hdgCorrRad);
    CalibrationStorage::ReadRollCalibration(rollCorrRad);
    CalibrationStorage::ReadPitchCalibration(pitchCorrRad);

    // Send PGN to requester
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(IMU_CALIBRATION_PGN);
    N2kMsg.Priority=2;
    N2kMsg.Add2ByteUInt((SCI_MFG_CODE << 5) | (0x03 << 3) | SCI_INDUSTRY_CODE);
    N2kMsg.Add2ByteDouble(hdgCorrRad,ANGLE_CAL_SCALE);
    N2kMsg.Add2ByteDouble(pitchCorrRad,ANGLE_CAL_SCALE);
    N2kMsg.Add2ByteDouble(rollCorrRad,ANGLE_CAL_SCALE);
    return NMEA2000.SendMsg(N2kMsg, DEV_IMU);
}


bool N2KHandler::ImuCalGroupFunctionHandler::ProcessRequest(const tN2kMsg &N2kMsg, uint32_t TransmissionInterval,
                                                            uint16_t TransmissionIntervalOffset,
                                                            uint8_t NumberOfParameterPairs, int Index, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::ImuCalGroupFunctionHandler::HandleRequest NumberOfParameterPairs=%d", NumberOfParameterPairs);
    return m_n2kHandler.SendImuCalValues();
}

bool N2KHandler::ImuCalGroupFunctionHandler::ProcessCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting,
                                                            uint8_t NumberOfParameterPairs, int Index, int iDev) {

    ESP_LOGI(TAG, "N2KHandler::ImuCalGroupFunctionHandler::HandleCommand NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int16_t value;
    for( int i=0; i < NumberOfParameterPairs; i++){
        uint8_t fn = N2kMsg.GetByte(Index);
        switch (fn){
            case 4: // Field 4: HeadingGOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "HeadingGOffset=%d", value);
                if (value == CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                    CalibrationStorage::UpdateHeadingCalibration(DEFAULT_ANGLE_CORR, false);
                }else{
                    CalibrationStorage::UpdateHeadingCalibration(value, true);
                }
                CalibrationStorage::ReadHeadingCalibration(m_n2kHandler.m_hdgCorrRad);
                break;
            case 5: // Field 5: PitchOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "PitchOffset=%d", value);
                if (value == CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                    CalibrationStorage::UpdatePitchCalibration(DEFAULT_ANGLE_CORR, false);
                }else{
                    CalibrationStorage::UpdatePitchCalibration(value, true);
                }
                CalibrationStorage::ReadPitchCalibration(m_n2kHandler.m_pitchCorrRad);
                break;
            case 6: // Field 6: RollOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "RollOffset=%d", value);
                if (value == CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                    CalibrationStorage::UpdateRollCalibration(DEFAULT_ANGLE_CORR, false);
                }else{
                    CalibrationStorage::UpdateRollCalibration(value, true);
                }
                CalibrationStorage::ReadRollCalibration(m_n2kHandler.m_rollCorrRad);
                break;
            default:
                break;
        }
    }
    return true;
}
