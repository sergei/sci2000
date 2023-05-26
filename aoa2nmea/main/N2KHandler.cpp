#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"

NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN,
                             TWAI_MODE, TWAI_TX_QUEUE_LEN);


static const char *TAG = "imu2nmea_N2KHandler";


N2KHandler::N2KHandler(const xQueueHandle &evtQueue, LEDBlinker &ledBlinker)
    :m_ledBlinker(ledBlinker)
    ,m_busListener(evtQueue, ledBlinker)
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
    uint32_t SerialNumberImu = ((uint32_t)chipid[0]) << 24 | ((uint32_t)chipid[1]) << 16 | ((uint32_t)chipid[2]) << 8 | ((uint32_t)chipid[3]);
    char SnoStrImu[33];
    snprintf(SnoStrImu, 32, "%lu", (long unsigned int)SerialNumberImu);

    NMEA2000.SetProductInformation(SnoStrImu, // Manufacturer's Model serial code
                                   PR_CODE,      // Manufacturer's product code
                                   MHU_MODEL_ID,     // Manufacturer's Model ID
                                   MFG_SW_VERSION,   // Manufacturer's Software version code
                                   MFG_MODEL_VER,// Manufacturer's Model version
                                   0xff,       // Load equivalency Default=1. x * 50 mA
                                   0xffff,         // N2K version Default=2101
                                   0xff,       // Certification level Default=1
                                   0
    );

    NMEA2000.SetConfigurationInformation("https://github.com/sergei/sci2000");
    NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly);
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
        // crank NMEA2000 state machine
        NMEA2000.ParseMessages();
//        for(int i = 0; i < m_listenerCount; i++){
//            m_listeners[i]->flush();
//        }
        while (true) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

    }
}

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
void N2KHandler::OnOpen() {
    // Start schedulers now.
}

N2KTwaiBusAlertListener::N2KTwaiBusAlertListener(QueueHandle_t const &evtQueue, LEDBlinker &ledBlinker)
        :m_evtQueue(evtQueue)
        ,m_ledBlinker(ledBlinker)
{
}

void N2KTwaiBusAlertListener::onAlert(uint32_t alerts, bool isError) {
    if ( isError ){
        ESP_LOGE(TAG, "CAN driver error");
        m_ledBlinker.SetBusState(false);
    }else{
        m_ledBlinker.SetBusState(true);
        // CAN bus available, crank the N2K FSM
        Event evt = {
                .src = CAN_DRIVER_EVENT,
                .isValid = true,
                .u= {.uiValue = alerts}

        };
        xQueueSend(m_evtQueue, &evt, 0);
    }
}


bool N2KHandler::addBusListener(TwaiBusListener *listener) {
    // Add listener to the list
    if (m_listenerCount < MAX_TWAI_LISTENERS) {
        m_listeners[m_listenerCount++] = listener;
    }

    return NMEA2000.addBusListener(listener);
}

void N2KHandler::onSideIfcTwaiFrame(unsigned long id, unsigned char len, const unsigned char *buf) {
    // Send frame to CAN bus
    // Prevent loopback by suspending side interface
    NMEA2000.SuspendSideInterface(true);
    NMEA2000.CANSendFrame(id, len, buf, false);
    NMEA2000.SuspendSideInterface(false);

    // Send frame to local NMEA200 stack
    NMEA2000.InjectSideTwaiFrame(id, len, buf);
}
