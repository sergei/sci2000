#ifndef IMU2NMEA_SLIPPACKET_H
#define IMU2NMEA_SLIPPACKET_H


static const int OUT_BUF_LEN = 64;
static const int SLIP_BUF_LEN = 128;

class SlipListener{
public:
    virtual void onPacketReceived(const unsigned char *buf, unsigned char len) = 0;
};

class ByteOutputStream{
public:
    virtual void sendEncodedBytes(unsigned char *buf, unsigned char len) = 0;
};

class SlipPacket{
public:
    SlipPacket(SlipListener &listener, ByteOutputStream &outStream)
            :listener(listener), outStream(outStream){}
    void EncodeAndSendPacket(const unsigned char *inputBuffer, unsigned char len);
    void onSlipByteReceived(unsigned char byte);

private:
    SlipListener &listener;
    ByteOutputStream &outStream;

    unsigned char slipBuffer[SLIP_BUF_LEN];
    unsigned char slipBufferIndex = 0;
    bool escapeNext = false;

    static const unsigned char SLIP_END = 0xC0;
    static const unsigned char SLIP_ESC = 0xDB;
    static const unsigned char SLIP_ESC_END = 0xDC;
    static const unsigned char SLIP_ESC_ESC = 0xDD;
};



#endif //IMU2NMEA_SLIPPACKET_H
