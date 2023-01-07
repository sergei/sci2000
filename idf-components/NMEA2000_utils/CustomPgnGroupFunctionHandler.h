#ifndef MHU2NMEA_CUSTOMPGNGROUPFUNCTIONHANDLER_H
#define MHU2NMEA_CUSTOMPGNGROUPFUNCTIONHANDLER_H

#include "NMEA2000.h"

/// Class to handle NMEA Group function commands sent by PGN 126208  for our custom PGNs
/// This task check that first two fields match the manufacturer code and industry code
/// Then it calls child class to process the actual request or command
class CustomPgnGroupFunctionHandler: public tN2kGroupFunctionHandler{
public:
    CustomPgnGroupFunctionHandler(tNMEA2000 *_pNMEA2000, unsigned long pgn, int mfgCode, int indCode)
    :tN2kGroupFunctionHandler(_pNMEA2000,pgn)
    ,m_mfgCode(mfgCode)
    ,m_indCode(indCode)
    {}
protected:
    /// Network sent a request
    /// Check if this request was sent to us
    bool HandleRequest(const tN2kMsg &N2kMsg,
                       uint32_t TransmissionInterval,
                       uint16_t TransmissionIntervalOffset,
                       uint8_t  NumberOfParameterPairs,
                       int iDev) final;

    /// Network sent a command
    /// Check if this command was sent to us
    bool HandleCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int iDev) final;

    /// Implement this method to process the request
    virtual bool ProcessRequest(const tN2kMsg &N2kMsg,
                                uint32_t TransmissionInterval,
                                uint16_t TransmissionIntervalOffset,
                                uint8_t  NumberOfParameterPairs,
                                int Index,
                                int iDev) =0;

    /// Implement this method to process the command
    virtual bool ProcessCommand(const tN2kMsg &N2kMsg, uint8_t PrioritySetting, uint8_t NumberOfParameterPairs, int Index, int iDev) = 0;
private:
    int m_mfgCode=0;
    int m_indCode=0;
};


#endif //MHU2NMEA_CUSTOMPGNGROUPFUNCTIONHANDLER_H
