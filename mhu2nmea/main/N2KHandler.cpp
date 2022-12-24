#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"
#include "CalibrationStorage.h"

NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN, TWAI_MODE_NORMAL);

static const unsigned long TX_PGNS_MHU[] PROGMEM={130306,   // Wind Speed
                                              MHU_CALIBRATION_PGN,
                                                  0};

static const unsigned long TX_PGNS_SPEED[] PROGMEM={   // Wind Speed
                                              128259L,  // Boat speed
                                              SPEED_CALIBRATION_PGN,
                                                0};

static const unsigned long RX_PGNS_MHU[] PROGMEM={
                                              0};

static const unsigned long RX_PGNS_SPEED[] PROGMEM={
                                              0};

static const char *TAG = "mhu2nmea_N2KHandler";

tN2kSyncScheduler N2KHandler::s_WindScheduler(false, 1000, 500);
tN2kSyncScheduler N2KHandler::s_WaterScheduler(false, 1000, 600);

N2KHandler::N2KHandler(const xQueueHandle &evtQueue)
    : m_evtQueue(evtQueue)
    , m_MhuCalGroupFunctionHandler(*this, &NMEA2000)
    , m_BoatSpeedCalGroupFunctionHandler(*this, &NMEA2000)
    , m_busListener(evtQueue)
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
    uint32_t SerialNumberMhu = ((uint32_t)chipid[0]) << 24 | ((uint32_t)chipid[1]) << 16 | ((uint32_t)chipid[2]) << 8 | ((uint32_t)chipid[3]);
    uint32_t SerialNumberSpeed = SerialNumberMhu + DEV_SPEED;
    char SnoStrMhu[33];
    snprintf(SnoStrMhu, 32, "%lu", (long unsigned int)SerialNumberMhu);

    char SnoStrSpeed[33];
    snprintf(SnoStrSpeed, 32, "%lu", (long unsigned int)SerialNumberSpeed);

    NMEA2000.SetProductInformation(SnoStrMhu, // Manufacturer's Model serial code
                                   MHU_PR_CODE,  // Manufacturer's product code
                                   MHU_MODEL_ID,     // Manufacturer's Model ID
                                   MFG_SW_VERSION,   // Manufacturer's Software version code
                                   MFG_MODEL_VER,// Manufacturer's Model version
                                   0xff,       // Load equivalency Default=1. x * 50 mA
                                   0xffff,         // N2K version Default=2101
                                   0xff,       // Certification level Default=1
                                   DEV_MHU
    );

    NMEA2000.SetProductInformation(SnoStrSpeed, // Manufacturer's Model serial code
                                   SPEED_PR_CODE,  // Manufacturer's product code
                                   SPEED_MODEL_ID,     // Manufacturer's Model ID
                                   MFG_SW_VERSION,     // Manufacturer's Software version code
                                   MFG_MODEL_VER,  // Manufacturer's Model version
                                   0xff,         // Load equivalency Default=1. x * 50 mA
                                   0xffff,           // N2K version Default=2101
                                   0xff,         // Certification level Default=1
                                   DEV_SPEED
    );

    // Set device information
    NMEA2000.SetDeviceInformation(SerialNumberMhu, // Unique number.
                                  130,             // Device function =  Atmospheric.
                                    85,              // Device class    = External Environment.
                               SCI_MFG_CODE ,
                                  SCI_INDUSTRY_CODE,
                                  DEV_MHU
    );

    // Set device information
    NMEA2000.SetDeviceInformation(SerialNumberSpeed, // Unique number.
                                  155,               // Device function = Devices that report water/ground speed. {Formerly Speed Sensors}
                                    60,                // Device class    = Navigation Equipment that provide information related to the passage of the vessel and potental obstructions/hazards {Formerly NavigationSystems}
                               SCI_MFG_CODE ,
                                  SCI_INDUSTRY_CODE,
                                  DEV_SPEED
    );

    NMEA2000.SetConfigurationInformation("https://github.com/sergei/sci2000");

    NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly);
    NMEA2000.ExtendTransmitMessages(TX_PGNS_MHU, DEV_MHU);
    NMEA2000.ExtendTransmitMessages(TX_PGNS_SPEED, DEV_SPEED);

    NMEA2000.ExtendReceiveMessages(RX_PGNS_MHU, DEV_MHU);
    NMEA2000.ExtendReceiveMessages(RX_PGNS_SPEED, DEV_SPEED);

    NMEA2000.AddGroupFunctionHandler(&m_MhuCalGroupFunctionHandler);
    NMEA2000.AddGroupFunctionHandler(&m_BoatSpeedCalGroupFunctionHandler);

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

    CalibrationStorage::ReadAwaCalibration(m_awaCorrRad);
    CalibrationStorage::ReadAwsCalibration(m_awsFactor);

    for( ;; ) {
        Event evt{};
        portBASE_TYPE res = xQueueReceive(m_evtQueue, &evt, 1);

        // Check if new  data is available
        if (res == pdTRUE) {
            switch (evt.src) {
                case AWS:
                    awsKts = evt.u.fValue * m_awsFactor;
                    isAwsValid = evt.isValid;
                    if( isAwsValid ){
                        awsUpdateTime = esp_timer_get_time();
                    }
                    break;
                case SOW:
                    sowKts = evt.u.fValue * m_sowFactor;
                    isSowValid = evt.isValid;
                    if( isSowValid ){
                        sowUpdateTime = esp_timer_get_time();
                    }
                    break;
                case AWA:
                    awaRad = evt.u.fValue + m_awaCorrRad;
                    isAwaValid = evt.isValid;
                    if ( isAwaValid ){
                        awaUpdateTime = esp_timer_get_time();
                    }
                    break;
                case CAN_DRIVER_EVENT:
                    break;
            }
        }

        int64_t now = esp_timer_get_time();
        // Check age and invalidate if it's too old
        if ( now - awsUpdateTime > AWS_TOUT){
            isAwsValid = false;
        }
        if ( now - awaUpdateTime > AWA_TOUT){
            isAwaValid = false;
        }
        if ( now - sowUpdateTime > SOW_TOUT){
            isSowValid = false;
        }

        // Check if it's time to send the messages
        if ( s_WindScheduler.IsTime() ) {
            s_WindScheduler.UpdateNextTime();

            tN2kMsg N2kMsg;
            double localAwsMs = isAwsValid ? KnotsToms(awsKts) : N2kDoubleNA;
            double localAwaRad = isAwaValid ? awaRad : N2kDoubleNA;
            tN2kWindReference windRef = !isAwsValid && !isAwaValid ? N2kWind_Unavailable : N2kWind_Apparent;

            SetN2kWindSpeed(N2kMsg, this->uc_WindSeqId++, localAwsMs, localAwaRad, windRef );
            bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_MHU);
            ESP_LOGI(TAG, "SetN2kWindSpeed AWS=%.0f AWA=%.1f ref=%d %s", msToKnots(localAwsMs), RadToDeg(localAwaRad), windRef, sentOk ? "OK" : "Failed");
        }

        if ( s_WaterScheduler.IsTime() ) {
            s_WaterScheduler.UpdateNextTime();
            tN2kMsg N2kMsg;

            double boatSpeed = isSowValid ? KnotsToms(sowKts) : N2kDoubleNA;
            SetN2kBoatSpeed(N2kMsg, this->uc_BoatSpeedSeqId++, boatSpeed, N2kDoubleNA, N2kSWRT_Paddle_wheel );
            bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_SPEED);
            ESP_LOGI(TAG, "SetN2kWindSpeed SOW=%.0f %s", msToKnots(boatSpeed), sentOk ? "OK" : "Failed");
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
    s_WaterScheduler.UpdateNextTime();
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
bool N2KHandler::SendMhuCalValues() {
    // Get calibration values
    float awaCorrRad, awsFactor;
    CalibrationStorage::ReadAwaCalibration(awaCorrRad);
    CalibrationStorage::ReadAwsCalibration(awsFactor);

    // Send PGN to requester
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(MHU_CALIBRATION_PGN);
    N2kMsg.Priority=2;
    N2kMsg.Add2ByteUInt((SCI_MFG_CODE << 5) | (0x03 << 3) | SCI_INDUSTRY_CODE);
    N2kMsg.Add2ByteDouble(awaCorrRad,AWA_CAL_SCALE);
    N2kMsg.Add2ByteDouble(awsFactor,AWS_CAL_SCALE);
    return NMEA2000.SendMsg(N2kMsg, DEV_MHU);
}

// Send PGN SPEED_CALIBRATION_PGN
bool N2KHandler::SendBoatSpeedCalValues() {
    // Get calibration values
    float speedFactor;
    CalibrationStorage::ReadSowCalibration(speedFactor);

    // Send PGN to requester
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(SPEED_CALIBRATION_PGN);
    N2kMsg.Priority=2;
    N2kMsg.Add2ByteUInt((SCI_MFG_CODE << 5) | (0x03 << 3) | SCI_INDUSTRY_CODE);
    N2kMsg.Add2ByteDouble(speedFactor, SOW_CAL_SCALE);
    return NMEA2000.SendMsg(N2kMsg, DEV_SPEED);
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
bool N2KHandler::CustomPgnGroupFunctionHandler::HandleRequest(const tN2kMsg &N2kMsg, uint32_t TransmissionInterval,
                                                              uint16_t TransmissionIntervalOffset,
                                                              uint8_t NumberOfParameterPairs, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::CustomPgnGroupFunctionHandler::HandleRequest NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int Index;
    StartParseRequestPairParameters(N2kMsg,Index);
    uint16_t mfgCode = 0xFFFF;
    uint8_t indCode = 0xFF;
    if ( NumberOfParameterPairs < 2)
        return false;

    for( int i=0; i < 2; i++) {
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
        return ProcessRequest(N2kMsg, TransmissionInterval, TransmissionIntervalOffset, NumberOfParameterPairs-2, Index, iDev);
    }else{
        ESP_LOGE(TAG, "Ignore request with mfgCode=%d indCode=%d", mfgCode, indCode);
        return false;
    }
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
bool N2KHandler::CustomPgnGroupFunctionHandler::HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting,
                                                              uint8_t NumberOfParameterPairs, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::CustomPgnGroupFunctionHandler::HandleCommand NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int Index;
    StartParseCommandPairParameters(N2kMsg,Index);
    uint16_t mfgCode = 0xFFFF;
    uint8_t indCode = 0xFF;
    if ( NumberOfParameterPairs < 2)
        return false;

    for( int i=0; i < 2; i++) {
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
            default:
                break;
        }
    }

    // Verify if it's this proprietary PGN belongs to us
    if ( mfgCode == SCI_MFG_CODE && indCode == SCI_INDUSTRY_CODE) {
        return ProcessCommand(N2kMsg,  PrioritySetting, NumberOfParameterPairs-2, Index, iDev);
    }else{
        ESP_LOGE(TAG, "Ignore request with mfgCode=%d indCode=%d", mfgCode, indCode);
        return false;
    }

}

bool N2KHandler::MhuCalGroupFunctionHandler::ProcessRequest(const tN2kMsg &N2kMsg, uint32_t TransmissionInterval,
                                                            uint16_t TransmissionIntervalOffset,
                                                            uint8_t NumberOfParameterPairs, int Index, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::MhuCalGroupFunctionHandler::HandleRequest NumberOfParameterPairs=%d", NumberOfParameterPairs);
    return m_n2kHandler.SendMhuCalValues();
}

bool N2KHandler::BoatSpeedCalGroupFunctionHandler::ProcessRequest(const tN2kMsg &N2kMsg, uint32_t TransmissionInterval,
                                                                  uint16_t TransmissionIntervalOffset,
                                                                  uint8_t NumberOfParameterPairs, int Index, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::BoatSpeedCalGroupFunctionHandler::HandleRequest NumberOfParameterPairs=%d", NumberOfParameterPairs);
    return m_n2kHandler.SendBoatSpeedCalValues();
}

bool N2KHandler::MhuCalGroupFunctionHandler::ProcessCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting,
                                                            uint8_t NumberOfParameterPairs, int Index, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::MhuCalGroupFunctionHandler::HandleCommand NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int16_t value;
    for( int i=0; i < NumberOfParameterPairs; i++){
        uint8_t fn = N2kMsg.GetByte(Index);
        switch (fn){
            case 4: // Field 4: AWAOffset, 2 bytes
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "AWAOffset=%d", value);
                if (value == CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                    CalibrationStorage::UpdateAwaCalibration(DEFAULT_ANGLE_CORR, false);
                }else{
                    CalibrationStorage::UpdateAwaCalibration(value, true);
                }
                CalibrationStorage::ReadAwaCalibration(m_n2kHandler.m_awaCorrRad);
                break;
            case 5: // Field 5: AWSMultiplier, 2 bytes
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "AWSMultiplier=%d", value);
                if (value == CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                    CalibrationStorage::UpdateAwsCalibration(DEFAULT_SPEED_FACTOR, false);
                }else{
                    CalibrationStorage::UpdateAwsCalibration(value, true);
                }
                CalibrationStorage::ReadAwsCalibration(m_n2kHandler.m_awsFactor);
                break;
            default:
                break;
        }
    }

    return true;
}

bool N2KHandler::BoatSpeedCalGroupFunctionHandler::ProcessCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting,
                                                                  uint8_t NumberOfParameterPairs, int Index, int iDev) {
    ESP_LOGI(TAG, "N2KHandler::BoatSpeedCalGroupFunctionHandler::HandleCommand NumberOfParameterPairs=%d", NumberOfParameterPairs);
    int16_t value;
    for( int i=0; i < NumberOfParameterPairs; i++){
        uint8_t fn = N2kMsg.GetByte(Index);
        switch (fn){ // NOLINT(hicpp-multiway-paths-covered)
            case 4: // Field 4: SOWMultiplier, 2 bytes
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "SOWMultiplier=%d", value);
                if (value == CALIBRATION_RESTORE_DEFAULT ){ // Restore default
                    CalibrationStorage::UpdateSowCalibration(DEFAULT_SPEED_FACTOR, false);
                }else{
                    CalibrationStorage::UpdateSowCalibration(value, true);
                }
                CalibrationStorage::ReadSowCalibration(m_n2kHandler.m_awsFactor);
                break;
            default:
                break;
        }
    }

    return true;
}
