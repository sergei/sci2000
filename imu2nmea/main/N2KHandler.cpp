#include <cmath>

#include <esp_log.h>

#include "N2KHandler.h"
#include "Event.hpp"
#include "CalibrationStorage.h"
#include "wmm.h"

NMEA2000_esp32_twai NMEA2000(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN,
                             TWAI_MODE_NORMAL, TWAI_TX_QUEUE_LEN);

static const unsigned long TX_PGNS_IMU[] PROGMEM={127250, // Vessel Heading
                                                  127257, // Attitude
                                                  126992, // System time
                                                  129025, // Position, Rapid Update
                                                  129026,  // COG & SOG, Rapid Update
                                                  127258,  // Magnetic Variation
                                                  IMU_CALIBRATION_PGN,
                                                  0};

static const unsigned long RX_PGNS_IMU[] PROGMEM={
                                              0};

static const char *TAG = "imu2nmea_N2KHandler";

tN2kSyncScheduler N2KHandler::s_HdgScheduler(false, DEFAULT_HDG_TX_RATE, 0);
tN2kSyncScheduler N2KHandler::s_AttScheduler(false, DEFAULT_ATTITUDE_TX_RATE, 0);

N2KHandler::N2KHandler(const xQueueHandle &evtQueue, LEDBlinker &ledBlinker, IMUCalInterface &imuCalInterface)
    :m_evtQueue(evtQueue)
    ,m_ledBlinker(ledBlinker)
    ,imuCalInterface(imuCalInterface)
    ,m_imuCalGroupFunctionHandler(*this, &NMEA2000)
    ,m_busListener(evtQueue, ledBlinker)
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

    NMEA2000.AddGroupFunctionHandler(&m_imuCalGroupFunctionHandler);
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
                    hdg = NormalizeDeg360((evt.u.imu.hdg + RadToDeg(m_hdgCorrRad)) - JAVELIN_COMPASS_MOUNT_OFFSET);
                    // NB Swap pitch and roll to match the way device mounted on Javelin bulkhead
                    // Roll must be positive when tilted right
                    roll = - NormalizeDeg180(evt.u.imu.pitch + RadToDeg(m_pitchCorrRad));
                    pitch = NormalizeDeg180(evt.u.imu.roll + RadToDeg(m_rollCorrRad));
                    calibrState = evt.u.imu.calibrState;
                    isImuValid = evt.isValid;
                    if( isImuValid ){
                        imuUpdateTime = esp_timer_get_time();
                    }
                    break;
                case GPS_DATA_RMC:
                    m_rmc = evt.u.gps.rmc;
                    gotRmc = true;
                    break;
                case GPS_DATA_GGA:
                    m_gga = evt.u.gps.gga;
                    gotGga = true;
                    break;
                case CAN_DRIVER_EVENT:
                    break;
            }

            if( gotRmc && gotGga ){
                transmitGpsData(m_rmc);
                gotRmc = false;
                gotGga = false;
            }
        }

        int64_t now = esp_timer_get_time();
        // Check age and invalidate if it's too old
        if ( now - imuUpdateTime > IMU_TOUT){
            isImuValid = false;
        }

        // Check if it's time to send the messages
        if ( s_HdgScheduler.IsTime() ) {
            s_HdgScheduler.UpdateNextTime();

            double localHdgRad = isImuValid ? DegToRad(hdg) : N2kDoubleNA;

            tN2kMsg N2kMsg;
            SetN2kMagneticHeading(N2kMsg, this->uc_SeqId, localHdgRad);
            N2kMsg.Priority = DEFAULT_HDG_TX_PRIO;
            bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
            m_ledBlinker.SetBusState(sentOk);
            ESP_LOGD(TAG, "SetN2kMagneticHeading HDG=%.1f  %s", RadToDeg(localHdgRad), sentOk ? "OK" : "Failed");
        }

        // Check if it's time to send the messages
        if ( s_AttScheduler.IsTime() ) {
            s_AttScheduler.UpdateNextTime();

            double localYawRad = N2kDoubleNA;  // Not quite sure what it's supposed to be referenced to. Just don't send it
            double localRollRad = isImuValid ? DegToRad(roll) : N2kDoubleNA;
            double localPitchRad = isImuValid ? DegToRad(pitch) : N2kDoubleNA;

            tN2kMsg N2kMsg;
            // Use heading sequence id to bin them together
            SetN2kAttitude(N2kMsg, this->uc_SeqId, localYawRad, localPitchRad, localRollRad);
            N2kMsg.Priority = DEFAULT_ATTITUDE_TX_PRIO;
            bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
            m_ledBlinker.SetBusState(sentOk);
            ESP_LOGD(TAG, "SetN2kAttitude HDG=%.1f PITCH=%.0f ROLL=%.0f %s", RadToDeg(localYawRad), RadToDeg(localPitchRad), RadToDeg(localRollRad), sentOk ? "OK" : "Failed");
        }

        this->uc_SeqId = (this->uc_SeqId + 1) % 253;

        // crank NMEA2000 state machine
        NMEA2000.ParseMessages();
    }
}

void N2KHandler::transmitFullGpsData(minmea_sentence_gga gga, uint16_t DaysSince1970) {

    double SecondsSinceMidnight = gga.time.hours * 3600 + gga.time.minutes * 60 + gga.time.seconds + gga.time.microseconds * 1e-6;
    double Latitude =  gga.fix_quality ?  minmea_tocoord(&gga.latitude) : N2kDoubleNA;
    double Longitude = gga.fix_quality ?  minmea_tocoord(&gga.longitude)  : N2kDoubleNA;
    double Altitude = gga.fix_quality ? minmea_tofloat(&gga.altitude) : N2kDoubleNA;
    tN2kGNSStype GNSStype = N2kGNSSt_GPS;
    auto GNSSmethod = (tN2kGNSSmethod) gga.fix_quality;
    unsigned char nSatellites = gga.satellites_tracked;
    double HDOP = gga.fix_quality ? minmea_tofloat(&gga.hdop) : N2kDoubleNA;

    tN2kMsg N2kMsg;
    SetN2kGNSS(N2kMsg, this->uc_SeqId, DaysSince1970,  SecondsSinceMidnight,
               Latitude,  Longitude,  Altitude,
               GNSStype,  GNSSmethod,
               nSatellites,  HDOP);

    bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
    m_ledBlinker.SetBusState(sentOk);
    ESP_LOGD(TAG, "SetN2kGNSS  %s", sentOk ? "OK" : "Failed");
}

void N2KHandler::transmitGpsData(const minmea_sentence_rmc &rmc)  {
    tN2kMsg N2kMsg;
    timespec ts = {};

    // seconds since midnight
    bool wholeSecFrame = true;
    uint16_t systemDate = 0;

    if(minmea_gettime(&ts, &rmc.date, &rmc.time) == 0 ) {
        wholeSecFrame = ts.tv_nsec == 0;
        systemDate = ts.tv_sec / SEC_IN_DAY; // Days since 1970-01-01
        double systemTime = fmod(ts.tv_sec + ts.tv_nsec * 1.e-9, SEC_IN_DAY);
        SetN2kSystemTime(N2kMsg, this->uc_SeqId, systemDate, systemTime);
        bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
        m_ledBlinker.SetBusState(sentOk);
        ESP_LOGD(TAG, "SetN2kSystemTime date=%d time=%.3f  %s", systemDate, systemTime, sentOk ? "OK" : "Failed");
    }

    double cog = rmc.valid ? DegToRad(minmea_tofloat(&rmc.course)) : N2kDoubleNA;
    double sog = rmc.valid ? KnotsToms(minmea_tofloat(&rmc.speed)): N2kDoubleNA;
    SetN2kCOGSOGRapid(N2kMsg, this->uc_SeqId, N2khr_true, cog, sog);
    bool sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
    m_ledBlinker.SetBusState(sentOk);
    ESP_LOGD(TAG, "SetN2kCOGSOGRapid cog=%.5f sog=%.5f  %s", cog, sog, sentOk ? "OK" : "Failed");

    if ( wholeSecFrame ){  // Send full GPS data
        transmitFullGpsData(m_gga, systemDate);
        if( rmc.valid) {
            if ( ! m_magDeclComputed ) {
                double year = systemDate / 365.25 + 1970;
                m_magDecl = computeMagDecl(minmea_tocoord(&m_gga.latitude), minmea_tocoord(&m_gga.longitude),  year);
                m_magDeclComputed = true;
            }

            // Magnetic variation on full second only
            double magVar = DegToRad(m_magDecl);
            SetN2kMagneticVariation(N2kMsg, this->uc_SeqId, N2kmagvar_WMM2020, systemDate, magVar);
            N2kMsg.Priority = 6;
            sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
            m_ledBlinker.SetBusState(sentOk);
            ESP_LOGD(TAG, "SetN2kMagneticVariation var=%.5f %s", magVar, sentOk ? "OK" : "Failed");
        }
    }else {  // Send rapid update
        double latitude = rmc.valid ? minmea_tocoord(&rmc.latitude) : N2kDoubleNA;
        double longitude = rmc.valid ? minmea_tocoord(&rmc.longitude) : N2kDoubleNA;
        SetN2kLatLonRapid(N2kMsg, latitude, longitude);
        sentOk = NMEA2000.SendMsg(N2kMsg, DEV_IMU);
        m_ledBlinker.SetBusState(sentOk);
        ESP_LOGD(TAG, "SetN2kLatLonRapid lat=%.5f time=%.5f  %s", latitude, longitude, sentOk ? "OK" : "Failed");
    }

}

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
void N2KHandler::OnOpen() {
    // Start schedulers now.
    s_HdgScheduler.UpdateNextTime();
    s_AttScheduler.UpdateNextTime();
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

bool N2KHandler::SendImuCalValues() const {
    // Get calibration values
    float hdgCorrRad, rollCorrRad, pitchCorrRad;
    CalibrationStorage::ReadHeadingCalibration(hdgCorrRad);
    CalibrationStorage::ReadRollCalibration(rollCorrRad);
    CalibrationStorage::ReadPitchCalibration(pitchCorrRad);

    // Send PGN to requester
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(IMU_CALIBRATION_PGN);
    N2kMsg.Priority=2;
    N2kMsg.Add2ByteUInt(SCI_IND_MFG_CODE);
    N2kMsg.Add2ByteDouble(RadToDeg(hdgCorrRad),ANGLE_CAL_SCALE);
    N2kMsg.Add2ByteDouble(RadToDeg(pitchCorrRad),ANGLE_CAL_SCALE);
    N2kMsg.Add2ByteDouble(RadToDeg(rollCorrRad),ANGLE_CAL_SCALE);
    N2kMsg.AddByte(calibrState);
    ESP_LOGI(TAG,"Send CAL hdg %.1f pitch %.1f roll %.1f calState %02X",
             RadToDeg(hdgCorrRad),
             RadToDeg(pitchCorrRad),
             RadToDeg(rollCorrRad),
             calibrState
             );
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
    uint8_t calCmd;
    for( int i=0; i < NumberOfParameterPairs; i++){
        uint8_t fn = N2kMsg.GetByte(Index);
        switch (fn){
            case 4: // Field 4: HeadingGOffset
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "HeadingGOffset=%d", value);
                CalibrationStorage::UpdateHeadingCalibration(value);
                CalibrationStorage::ReadHeadingCalibration(m_n2kHandler.m_hdgCorrRad);
                break;
            case 5: // Field 5: PitchOffset
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "PitchOffset=%d", value);
                CalibrationStorage::UpdatePitchCalibration(value);
                CalibrationStorage::ReadPitchCalibration(m_n2kHandler.m_pitchCorrRad);
                break;
            case 6: // Field 6: RollOffset, 2 bytes 0xfffe - restore default 0xFFFF - leave unchanged
                value = N2kMsg.Get2ByteInt(Index);
                ESP_LOGI(TAG, "RollOffset=%d", value);
                CalibrationStorage::UpdateRollCalibration(value);
                CalibrationStorage::ReadRollCalibration(m_n2kHandler.m_rollCorrRad);
                break;
            case 7: // Field 7: CMPS12 Calibration: FD - Store, FE - Erase, FF - Leave unchanged
                calCmd = N2kMsg.GetByte(Index);
                ESP_LOGI(TAG, "Cal CMD=%02X", calCmd);
                if ( calCmd == Cmps12CalState::CMPS12_STORE_CAL ){
                    m_n2kHandler.imuCalInterface.StoreCalibration();
                }else if ( calCmd == Cmps12CalState::CMPS12_ERASE_CAL ){
                    m_n2kHandler.imuCalInterface.EraseCalibration();
                }
                break;
            default:
                break;
        }
    }
    return true;
}

float N2KHandler::NormalizeDeg360(double deg) {
    while (deg < 0) deg += 360.f;
    while (deg >= 360) deg -= 360.f;

    return (float)deg;
}

float N2KHandler::NormalizeDeg180(double deg) {
    deg = NormalizeDeg360(deg);
    if (deg > 180) deg -= 360.f;
    return (float)deg;
}

bool N2KHandler::addBusListener(TwaiBusListener *listener) {
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
