#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"
#include "CalibrationStorage.h"

//tNMEA2000_esp32 NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN);
NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN, TWAI_MODE_NORMAL);

static const unsigned long TX_PGNS[] PROGMEM={130306,  // Wind Speed
                                              MHU_CALIBRATION_PGN,
                                                0};

static const unsigned long RX_PGNS[] PROGMEM={MHU_CALIBRATION_PGN,
                                              0};

static const char *TAG = "mhu2nmea_N2KHandler";

tN2kSyncScheduler N2KHandler::s_WindScheduler(false, 1000, 500);

N2KHandler::N2KHandler(const xQueueHandle &evtQueue)
    : m_evtQueue(evtQueue)
    , m_MhuCalGroupFunctionHandler(*this, &NMEA2000)
    , m_busListener(evtQueue)
{
}

void N2KHandler::Init() {

    esp_log_level_set("NMEA2000_esp32_twai", ESP_LOG_DEBUG); // enable DEBUG logs from ESP32_TWAI layer
    NMEA2000.SetForwardStream(&debugStream);         // PC output on idf monitor
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text
    NMEA2000.EnableForward(true);                       // Disable all msg forwarding to USB (=Serial)

    NMEA2000.setBusEventListener(&m_busListener);

    NMEA2000.SetN2kCANMsgBufSize(8);
    NMEA2000.SetN2kCANReceiveFrameBufSize(100);

    uint8_t chipid[8];
    esp_efuse_mac_get_default(chipid);
    uint32_t SerialNumber = ((uint32_t)chipid[0]) << 24 | ((uint32_t)chipid[1]) << 16 | ((uint32_t)chipid[2]) << 8 | ((uint32_t)chipid[3]);
    char SnoStr[33];
    snprintf(SnoStr,32,"%lu",(long unsigned int)SerialNumber);

    NMEA2000.SetProductInformation(SnoStr,              // Manufacturer's Model serial code
                                   130,                    // Manufacturer's product code
                                   "SCI MHU->N2K",            // Manufacturer's Model ID
                                   "1.0.0.0 (2022-12-07)",    // Manufacturer's Software version code
                                   "1.0.0.0 (2022-12-07)" // Manufacturer's Model version
    );
    // Det device information
    NMEA2000.SetDeviceInformation(SerialNumber, // Unique number. Use e.g. Serial number.
                                  130,    // Device function =  Atmospheric. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                    85,       // Device class     = External Environment. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                               SCI_MFG_CODE 
    );

    NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly);
    NMEA2000.ExtendTransmitMessages(TX_PGNS);
    NMEA2000.ExtendReceiveMessages(RX_PGNS);
    NMEA2000.ExtendSingleFrameMessages(RX_PGNS);

    NMEA2000.AddGroupFunctionHandler(&m_MhuCalGroupFunctionHandler);
    NMEA2000.SetOnOpen(OnOpen);
}

static void n2k_task( void *me ) {
    ((N2KHandler *)me)->N2KTask();
}

void N2KHandler::StartTask() {
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

    CalibrationStorage::ReadCalibration(m_awaCorrRad, m_awsFactor);

    for( ;; ) {
        Event evt{};
        portBASE_TYPE res = xQueueReceive(m_evtQueue, &evt, 1);

        // Check if new  data is available
        if (res == pdTRUE) {
            switch (evt.src) {
                case AWS:
                    awsKts = evt.u.fValue * m_awsFactor;  // FIXME convert Hz to Kts
                    isAwsValid = evt.isValid;
                    break;
                case AWA:
                    awaRad = evt.u.fValue + m_awaCorrRad;
                    isAwaValid = evt.isValid;
                    break;
                case CAN_DRIVER_EVENT:
                    ESP_LOGI(TAG, "Bus event, just crank FSM");
                    break;
            }
        }

        // Check if it's time to send the message
        if ( s_WindScheduler.IsTime() ) {
            s_WindScheduler.UpdateNextTime();

            tN2kMsg N2kMsg;
            double localAwsMs = isAwsValid ? KnotsToms(awsKts) : N2kDoubleNA;
            double localAwaRad = isAwaValid ? awaRad : N2kDoubleNA;
            tN2kWindReference windRef = !isAwsValid && !isAwaValid ? N2kWind_Unavailable : N2kWind_Apparent;

            SetN2kWindSpeed(N2kMsg, this->uc_WindSeqId++, localAwsMs, localAwaRad, windRef );
            bool sentOk = NMEA2000.SendMsg(N2kMsg);
            ESP_LOGI(TAG, "SetN2kWindSpeed AWS=%.0f AWA=%.1f ref=%d %s", msToKnots(localAwsMs), RadToDeg(localAwaRad), windRef, sentOk ? "OK" : "Failed");
        }

        // crank NMEA2000 state machine
        NMEA2000.ParseMessages();
    }
}

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
void N2KHandler::OnOpen() {
    // Start schedulers now.
    s_WindScheduler.UpdateNextTime();
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


// Send PGN GET_MHU_CALIBRATION_PGN
bool N2KHandler::SendCalValues() {
    // Get calibration values
    float awaCorrRad, awsFactor;
    CalibrationStorage::ReadCalibration(awaCorrRad, awsFactor);

    // Send PGN to requester
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(MHU_CALIBRATION_PGN);
    N2kMsg.Priority=2;
    N2kMsg.Add2ByteUInt((SCI_MFG_CODE << 5) | (0x03 << 3) | SCI_INDUSTRY_CODE);
    N2kMsg.Add2ByteDouble(awaCorrRad,AWA_CAL_SCALE);
    N2kMsg.Add2ByteDouble(awsFactor,AWS_CAL_SCALE);
    return NMEA2000.SendMsg(N2kMsg);
}

/*
Your PGN will be then requested by using PGN 126208 and by setting fields:
Field 1: Request Group Function Code = 0 (Request Message), 8 bits
Field 2: PGN = 130900, 24 bits
Field 3: Transmission interval = FFFF FFFF (Do not change interval), 32 bits
Field 4: Transmission interval offset = 0xFFFF (Do not change offset), 16 bits
Field 5: Number of Pairs of Request Parameters to follow = 2, 8 bits
Field 6: Field number of requested parameter = 1, 8 bits
Field 7: Value of requested parameter = mfg code, 16 bits
Field 8: Field number of requested parameter = 3, 8 bits
Field 9: Value of requested parameter = 4 (Marine), 8 bits
 */
bool N2KHandler::MhuCalGroupFunctionHandler::HandleRequest(const tN2kMsg &N2kMsg, uint32_t TransmissionInterval,
                                                           uint16_t TransmissionIntervalOffset,
                                                           uint8_t NumberOfParameterPairs, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::MhuCalGroupFunctionHandler::HandleRequest NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int Index;
    StartParseRequestPairParameters(N2kMsg,Index);
    uint16_t mfgCode = 0xFFFF;
    uint8_t indCode = 0xFF;
    for( int i=0; i < NumberOfParameterPairs; i++) {
        uint8_t fn = N2kMsg.GetByte(Index);
        switch (fn) {
            case 1: // Mfg code
                mfgCode = N2kMsg.Get2ByteUInt(Index);
                ESP_LOGI(TAG, "mfgCode=%d", mfgCode);
                break;
            case 3: // Industry code
                indCode = N2kMsg.GetByte(Index);
                ESP_LOGI(TAG, "indCode=%d", indCode);
                break;
            default:
                break;
        }
    }

    // Verify if it's this proprietary PGN belongs to us
    if ( mfgCode == SCI_MFG_CODE && indCode == SCI_INDUSTRY_CODE) {
        m_n2kHandler.SendCalValues();
    }else{
        ESP_LOGE(TAG, "Ignore request with mfgCode=%d indCode=%d", mfgCode, indCode);
    }
    return false;
}

/*
Field 1: Request Group Function Code = 1 (Command Message), 8 bits
Field 2: PGN = 130900, 24 bits
Field 3: Priority Setting = 0x8 (do not change priority), 4 bits
Field 4: Reserved =0xf, 4 bits
Field 5: Number of Pairs of Request Parameters to follow = 3, 8 bits
Field 6: Field number of commanded parameter = 1, 8 bits
Field 7: Value of commanded parameter = mfg code, 16 bits
Field 8: Field number of commanded parameter = 3, 8 bits
Field 9: Value of commanded parameter = 4 (Marine), 8 bits
Field 10: Field number of commanded parameter = 4, 8 bits
Field 11: Value of commanded parameter = AWSOffset, 16 bits
 */
bool N2KHandler::MhuCalGroupFunctionHandler::HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting,
                                                           uint8_t NumberOfParameterPairs, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::MhuCalGroupFunctionHandler::HandleCommand NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int Index;
    StartParseCommandPairParameters(N2kMsg,Index);
    uint16_t mfgCode = 0xFFFF;
    uint8_t indCode = 0xFF;
    int16_t value;
    bool calWasUpdated = false;
    for( int i=0; i < NumberOfParameterPairs; i++){
        uint8_t fn = N2kMsg.GetByte(Index);
        switch (fn){
            case 1: // Mfg code
                mfgCode = N2kMsg.Get2ByteUInt(Index);
                ESP_LOGI(TAG, "mfgCode=%d", mfgCode);
                break;
            case 3: // Industry code
                indCode = N2kMsg.GetByte(Index);
                ESP_LOGI(TAG, "indCode=%d", indCode);
                break;
            case 4: // Field 4: AWAOffset, 2 bytes
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "AWAOffset=%d", value);
                if ( mfgCode == SCI_MFG_CODE && indCode == SCI_INDUSTRY_CODE) { // This PGN belongs to us
                    if (value == MHU_CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                        CalibrationStorage::UpdateAwaCalibration(DEFAULT_AWA_CORR, false);
                    }else{
                        CalibrationStorage::UpdateAwaCalibration(value, true);
                    }
                    calWasUpdated = true;
                }
                break;
            case 5: // AWSMultiplier, 2 bytes
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "AWSMultiplier=%d", value);
                if ( mfgCode == SCI_MFG_CODE && indCode == SCI_INDUSTRY_CODE) { // This PGN belongs to us
                    if (value == MHU_CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                        CalibrationStorage::UpdateAwsCalibration(DEFAULT_AWS_CORR, false);
                    }else{
                        CalibrationStorage::UpdateAwsCalibration(value, true);
                    }
                    calWasUpdated = true;
                }
                break;
            default:
                break;
        }
    }

    if ( calWasUpdated ){
        CalibrationStorage::ReadCalibration(m_n2kHandler.m_awaCorrRad, m_n2kHandler.m_awsFactor);
    }

    return false;
}
