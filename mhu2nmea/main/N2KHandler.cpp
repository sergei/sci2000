#include "N2KHandler.h"
#include "NMEA2000_esp32.h"
#include "N2kMessages.h"

tNMEA2000_esp32 NMEA2000;

const unsigned long TransmitMessages[] PROGMEM={130306,  // Wind Speed
                                                0};

void N2KHandler::Init() {

    NMEA2000.SetForwardStream(&debugStream);         // PC output on idf monitor
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text
    NMEA2000.EnableForward(true);                       // Disable all msg forwarding to USB (=Serial)


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

    NMEA2000.ExtendTransmitMessages(TransmitMessages);
}


void N2KHandler::SetAwa(bool isValid, float value) {
    xSemaphoreTake(awaMutex, portMAX_DELAY);
    isAwaValid = isValid;
    awaRad = value;
    awaUpdateTime = esp_timer_get_time();
    xSemaphoreGive(awaMutex);
}

void N2KHandler::SetAws(bool isValid, float value) {
    xSemaphoreTake(awsMutex, portMAX_DELAY);
    isAwsValid = isValid;
    awsKts = value;
    awsUpdateTime = esp_timer_get_time();
    xSemaphoreGive(awsMutex);
}

N2KHandler::N2KHandler() {
    awaMutex = xSemaphoreCreateMutex();
    awsMutex = xSemaphoreCreateMutex();
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
    Init();
    for( ;; ) {
        if ( isAwsValid && isAwaValid ) {
            tN2kMsg N2kMsg;
            double localAwaRad;
            double localAwsMs;
            xSemaphoreTake(awsMutex, portMAX_DELAY);
            localAwsMs = KnotsToms(awsKts);
            xSemaphoreGive(awsMutex);
            xSemaphoreTake(awaMutex, portMAX_DELAY);
            localAwaRad = awaRad;
            xSemaphoreGive(awaMutex);


            SetN2kWindSpeed(N2kMsg, this->uc_WindSeqId++, localAwsMs, localAwaRad, N2kWind_Apparent );
            NMEA2000.SendMsg(N2kMsg);
        }
        NMEA2000.ParseMessages();
        vTaskDelay(500 / portTICK_PERIOD_MS); // 500 mS

    }
}
