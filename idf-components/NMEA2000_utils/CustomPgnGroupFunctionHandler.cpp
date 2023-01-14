#include <esp_log.h>
#include "CustomPgnGroupFunctionHandler.h"

static const char *TAG = "mhu2nmea_N2KHandler";

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
bool CustomPgnGroupFunctionHandler::HandleRequest(const tN2kMsg &N2kMsg, uint32_t TransmissionInterval,
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
    if ( mfgCode == m_mfgCode && indCode == m_indCode) {
        return ProcessRequest(N2kMsg, TransmissionInterval, TransmissionIntervalOffset, NumberOfParameterPairs-2, Index, iDev);
    }else{
        ESP_LOGE(TAG, "Ignore request with mfgCode=%d indCode=%d expected %d %d ", mfgCode, indCode, m_mfgCode, m_indCode);
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
 */
bool CustomPgnGroupFunctionHandler::HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting,
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
    if ( mfgCode == m_mfgCode && indCode == m_indCode) {
        return ProcessCommand(N2kMsg,  PrioritySetting, NumberOfParameterPairs-2, Index, iDev);
    }else{
        ESP_LOGE(TAG, "Ignore request with mfgCode=%d indCode=%d", mfgCode, indCode);
        return false;
    }

}

