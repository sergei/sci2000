#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"
#include "CalibrationStorage.h"

//tNMEA2000_esp32 NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN);
NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN, TWAI_MODE_NORMAL);

static const unsigned long TX_PGNS[] PROGMEM={130306,  // Wind Speed
                                              GET_MHU_CALIBRATION_PGN,
                                                0};

static const unsigned long RX_PGNS[] PROGMEM={SET_MHU_CALIBRATION_PGN,
                                                0};

static const char *TAG = "mhu2nmea_N2KHandler";

tN2kSyncScheduler N2KHandler::s_WindScheduler(false, 1000, 500);

static bool SendCalValuesPgn();

static bool ISORequestHandler(unsigned long RequestedPGN, unsigned char , int ){
    ESP_LOGI(TAG, "Got ISO request for PGN %ld", RequestedPGN);
    if ( RequestedPGN == GET_MHU_CALIBRATION_PGN) {
        return SendCalValuesPgn();
    }
    return false;
}

N2KHandler::N2KHandler(const xQueueHandle &evtQueue)
:m_evtQueue(evtQueue)
,m_calMsgHandler(evtQueue,SET_MHU_CALIBRATION_PGN, &NMEA2000)
,m_busListener(evtQueue)
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
                                  2020 // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );

    NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly);
    NMEA2000.ExtendTransmitMessages(TX_PGNS);
    NMEA2000.ExtendReceiveMessages(RX_PGNS);
    NMEA2000.ExtendSingleFrameMessages(RX_PGNS);

    NMEA2000.AttachMsgHandler(&m_calMsgHandler);
    NMEA2000.SetISORqstHandler(ISORequestHandler);
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
                case CALIBRATION_UPDATED:
                    CalibrationStorage::ReadCalibration(m_awaCorrRad, m_awsFactor);
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

CalibrationMessageHandler::CalibrationMessageHandler(QueueHandle_t const &evtQueue, unsigned long pgn, tNMEA2000 *pNMEA2000)
        :tNMEA2000::tMsgHandler(pgn, pNMEA2000)
        ,m_evtQueue(evtQueue)
{
}

void CalibrationMessageHandler::HandleMsg(const tN2kMsg &N2kMsg) {
    if ( N2kMsg.PGN == SET_MHU_CALIBRATION_PGN){
        int Index=0;
        unsigned char sid = N2kMsg.GetByte(Index);
        auto dest = (MHU_CALIBR_DEST)N2kMsg.GetByte(Index);
        int16_t value = N2kMsg.Get2ByteInt(Index);

        ESP_LOGI(TAG, "Got calibration pgn %ld(%08lX), sid=%d, des=%d, value = %d ", N2kMsg.PGN, N2kMsg.PGN, sid, dest, value);

        switch( dest ) {
            case MHU_CALIBR_SET_AWA:
                CalibrationStorage::UpdateAwaCalibration(value);
                break;
            case MHU_CALIBR_CLEAR_AWA:
                CalibrationStorage::UpdateAwaCalibration(DEFAULT_AWA_CORR);
                break;
            case MHU_CALIBR_SET_AWS:
                CalibrationStorage::UpdateAwsCalibration(value);
                break;
            case MHU_CALIBR_CLEAR_AWS:
                CalibrationStorage::UpdateAwsCalibration(DEFAULT_AWS_CORR);
                break;
        }

        Event evt = {
            .src = CALIBRATION_UPDATED,
            .isValid = true,
            .u = {}
        };
        xQueueSend(m_evtQueue, &evt, 0);
    }
}

// Send PGN GET_MHU_CALIBRATION_PGN
static bool SendCalValuesPgn() {
    // Get calibration values
    float awaCorrRad, awsFactor;
    CalibrationStorage::ReadCalibration(awaCorrRad, awsFactor);

    // Send PGN to requester
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(GET_MHU_CALIBRATION_PGN);
    N2kMsg.Priority=2;
    N2kMsg.AddByte(0);
    N2kMsg.Add2ByteDouble(awaCorrRad,AWA_CAL_SCALE);
    N2kMsg.Add2ByteDouble(awsFactor,AWS_CAL_SCALE);
    return NMEA2000.SendMsg(N2kMsg);
}



