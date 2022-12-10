#include "N2KHandler.h"
#include "NMEA2000_esp32.h"
#include "N2kMessages.h"

tNMEA2000_esp32 NMEA2000;

const unsigned long TransmitMessages[] PROGMEM={130306,  // Wind Speed
                                                0};

void N2KHandler::Init() {

//  NMEA2000.SetForwardStream(&Serial);            // PC output on due native port
//  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text
// NMEA2000.EnableForward(false);                  // Disable all msg forwarding to USB (=Serial)


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

void N2KHandler::Update(bool isValid, double awsKts, double awaDeg) {
    if ( isValid ) {
        tN2kMsg N2kMsg;
        SetN2kWindSpeed(N2kMsg, this->uc_WindSeqId++, KnotsToms(awsKts), DegToRad(awaDeg), N2kWind_Apparent );
        NMEA2000.SendMsg(N2kMsg);
    }
    NMEA2000.ParseMessages();
}
