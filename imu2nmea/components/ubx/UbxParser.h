#ifndef IMU2NMEA_UBXPARSER_H
#define IMU2NMEA_UBXPARSER_H

#include "ubx_defs.h"

class UbxParser {

public:

class Writer{
public:
    virtual void writeToUbx(const uint8_t *b, size_t len)=0;
};

class Listener{
public:
    virtual void onUbxMsg(uint8_t cls, uint8_t type, UBX_message_t msg) = 0;
};

public:
    explicit UbxParser(Writer &writer, Listener &listener);
    bool send_message(uint8_t msg_class, uint8_t msg_id, UBX_message_t &message, uint16_t len);
    bool read(uint8_t byte);

private:
    static void calculate_checksum(uint8_t msg_cls, uint8_t msg_id, uint16_t len, UBX_message_t payload,
                       uint8_t &ck_a, uint8_t &ck_b) ;

    void write(uint8_t b);
    void write(const uint8_t *b, size_t len);
    void restart();

private:
    Writer &m_writer;
    Listener &m_listener;

    // Parsing State Working Memory
    uint8_t prev_byte_;
    uint16_t buffer_head_ = 0;
    bool start_message_ = false;
    bool end_message_ = false;
    bool got_ack_ = false;
    bool got_ver_ = false;
    bool got_nack_ = false;
    parse_state_t parse_state_;
    uint8_t message_class_;
    uint8_t message_type_;
    uint16_t length_;
    uint8_t ck_a_;
    uint8_t ck_b_;
    uint32_t num_errors_ = 0;
    uint32_t num_messages_received_ = 0;

    // Main buffers for communication
    UBX_message_t out_message_;
    UBX_message_t in_message_;


    bool decode_message();
};


#endif //IMU2NMEA_UBXPARSER_H
