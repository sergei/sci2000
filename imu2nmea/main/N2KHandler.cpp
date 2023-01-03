#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"

NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN, TWAI_MODE_NORMAL);

static const unsigned long TX_PGNS_IMU[] PROGMEM={127250, // Vessel Heading
                                                  127257, // Attitude
                                                  0};

static const unsigned long RX_PGNS_IMU[] PROGMEM={
                                              0};

static const char *TAG = "imu2nmea_N2KHandler";

tN2kSyncScheduler N2KHandler::s_ImuScheduler(false, 200, 0);

N2KHandler::N2KHandler(const xQueueHandle &evtQueue)
    : m_evtQueue(evtQueue)
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

    for( ;; ) {
        Event evt{};
        portBASE_TYPE res = xQueueReceive(m_evtQueue, &evt, 1);

        // Check if new  data is available
        if (res == pdTRUE) {
            switch (evt.src) {
                case IMU:
                    hdg = evt.u.imu.hdg;
                    pitch = evt.u.imu.pitch;
                    roll = evt.u.imu.roll;
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
            ESP_LOGD(TAG, "SetN2kMagneticHeading HDG=%.1f  %s", RadToDeg(localHdgRad), sentOk ? "OK" : "Failed");

            double localRollRad = isImuValid ? DegToRad(roll) : N2kDoubleNA;
            double localPitchRad = isImuValid ? DegToRad(pitch) : N2kDoubleNA;

            SetN2kAttitude(N2kMsg, this->uc_ImuSeqId++, localHdgRad,localPitchRad, localRollRad);
            sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
            ESP_LOGD(TAG, "SetN2kMagneticHeading HDG=%.1f PITCH=%.0f ROLL=%.0f %s", RadToDeg(localHdgRad), RadToDeg(localPitchRad), RadToDeg(localRollRad), sentOk ? "OK" : "Failed");
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

