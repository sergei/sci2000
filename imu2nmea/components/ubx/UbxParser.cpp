#include <cstddef>
#include <esp_log.h>
#include "UbxParser.h"

static const char *TAG = "imu2nmea_GPSHandler";

UbxParser::UbxParser(UbxParser::Writer &writer, Listener &listener)
    :m_writer(writer)
    ,m_listener(listener)
{
}

bool UbxParser::send_message(uint8_t msg_class, uint8_t msg_id, UBX_message_t &message, uint16_t len)
{
    // First, calculate the checksum
    uint8_t ck_a, ck_b;
    calculate_checksum(msg_class, msg_id, len, message, ck_a, ck_b);

    // Send message
    write(START_BYTE_1);
    write(START_BYTE_2);
    write(msg_class);
    write(msg_id);
    write(len & 0xFF);
    write((len >> 8) & 0xFF);
    write(message.buffer, len);
    write(ck_a);
    write(ck_b);
    return true;
}

void UbxParser::calculate_checksum(const uint8_t msg_cls,
                             const uint8_t msg_id,
                             const uint16_t len,
                             const UBX_message_t payload,
                             uint8_t &ck_a,
                             uint8_t &ck_b)
{
    ck_a = ck_b = 0;

    // Add in class
    ck_a += msg_cls;
    ck_b += ck_a;

    // Id
    ck_a += msg_id;
    ck_b += ck_a;

    // Length
    ck_a += len & 0xFF;
    ck_b += ck_a;
    ck_a += (len >> 8) & 0xFF;
    ck_b += ck_a;

    // Payload
    for (int i = 0; i < len; i++)
    {
        ck_a += payload.buffer[i];
        ck_b += ck_a;
    }
}

void UbxParser::write(const uint8_t b) {
    write(&b, 1);
}

void UbxParser::write(const uint8_t *b, size_t len) {
    m_writer.writeToUbx(b, len);
}


bool UbxParser::read(uint8_t byte)
{
    switch (parse_state_)
    {
        case START:
            if (byte == START_BYTE_2 && prev_byte_ == START_BYTE_1)
            {
                buffer_head_ = 0;
                parse_state_ = GOT_START_FRAME;
                message_class_ = 0;
                message_type_ = 0;
                length_ = 0;
                ck_a_ = 0;
                ck_b_ = 0;
                start_message_ = true;
                end_message_ = false;
            }
            break;
        case GOT_START_FRAME:
            message_class_ = byte;
            parse_state_ = GOT_CLASS;
            break;
        case GOT_CLASS:
            message_type_ = byte;
            parse_state_ = GOT_MSG_ID;
            break;
        case GOT_MSG_ID:
            length_ = byte;
            parse_state_ = GOT_LENGTH1;
            break;
        case GOT_LENGTH1:
            length_ |= (uint16_t)byte << 8;
            parse_state_ = GOT_LENGTH2;
            if (length_ > BUFFER_SIZE)
            {
                ESP_LOGI(TAG, "Message 0x%x-0x%x is too long (%d > %zu)", message_class_, message_type_,
                    length_, BUFFER_SIZE);
                num_errors_++;
                prev_byte_ = byte;
                restart();
                return false;
            }
            break;
        case GOT_LENGTH2:
            if (buffer_head_ < length_)
            {
                // push the byte onto the data buffer
                in_message_.buffer[buffer_head_] = byte;
                if (buffer_head_ == length_ - 1)
                {
                    parse_state_ = GOT_PAYLOAD;
                }
                buffer_head_++;
            }
            break;
        case GOT_PAYLOAD:
            ck_a_ = byte;
            parse_state_ = GOT_CK_A;
            break;
        case GOT_CK_A:
            ck_b_ = byte;
            parse_state_ = GOT_CK_B;
            break;
        default:
            num_errors_++;
            parse_state_ = START;
            end_message_ = false;
            start_message_ = false;
            break;
    }

    // If we have a complete packet, then try to parse it
    if (parse_state_ == GOT_CK_B)
    {
        if (decode_message())
        {
            parse_state_ = START;
            end_message_ = true;
            start_message_ = false;
            prev_byte_ = byte;
            return true;
        }
        else
        {
            // indicate error if it didn't work
            ESP_LOGI(TAG, "\n failed to parse message");
            num_errors_++;
            parse_state_ = START;
            start_message_ = false;
            end_message_ = false;
        }
    }
    prev_byte_ = byte;
    return false;
}

void UbxParser::restart()
{
    parse_state_ = START;
    end_message_ = false;
    start_message_ = false;
}

bool UbxParser::decode_message()
{
    // First, check the checksum
    uint8_t ck_a, ck_b;
    calculate_checksum(message_class_, message_type_, length_, in_message_, ck_a, ck_b);
    if (ck_a != ck_a_ || ck_b != ck_b_)
        return false;
    uint8_t version;  // 0 poll request, 1 poll (receiver to return config data key
    // and value pairs)
    uint8_t layer;
    uint8_t reserved1[2];
    uint32_t cfgDataKey;
    uint64_t cfgData;
    num_messages_received_++;

    // Parse the payload
    switch (message_class_)
    {
        case CLASS_ACK:
            ESP_LOGI(TAG, "ACK_");
            switch (message_type_)
            {
                case ACK_ACK:
                    got_ack_ = true;
                    ESP_LOGI(TAG, "ACK");
                    break;
                case ACK_NACK:
                    got_nack_ = true;
                    ESP_LOGI(TAG, "NACK");
                    break;
                default:
                    ESP_LOGI(TAG, "%d", message_type_);
                    break;
            }
            break;
        case CLASS_MON:
            ESP_LOGI(TAG, "MON_");
            switch (message_type_)
            {
                case MON_VER:
                    ESP_LOGI(TAG, "VER");
                    break;
                case MON_COMMS:

                    ESP_LOGI(TAG, "COMMS");
                    break;
                case MON_TXBUF:

                    ESP_LOGI(TAG, "TXMON_TXBUF");
                    break;
            }
            break;
        case CLASS_RXM:
            ESP_LOGI(TAG, "RXM_");
            switch (message_type_)
            {
                case RXM_RAWX:
                    ESP_LOGI(TAG, "RAWX");
                    break;
                case RXM_SFRBX:
                    ESP_LOGI(TAG, "SFRBX");
                    break;
            }
            break;
        case CLASS_NAV:
            ESP_LOGI(TAG, "NAV_");
            switch (message_type_)
            {
                case NAV_PVT:
                    ESP_LOGI(TAG, "PVT ");
                    break;
                case NAV_RELPOSNED:
                    ESP_LOGI(TAG, "RELPOSNED ");
                    break;
                case NAV_POSECEF:
                    ESP_LOGI(TAG, "POSECEF");
                    break;
                case NAV_VELECEF:
                    ESP_LOGI(TAG, "VELECEF");
                    break;
                default:
                    ESP_LOGI(TAG, " unknown (%d) ", message_type_);
            }
            break;
        case CLASS_CFG:  // only needed for getting data
            ESP_LOGI(TAG, "CFG_");
            switch (message_type_)
            {
                case CFG_VALGET:
                {
                    ESP_LOGI(TAG, "VALGET = ");
                    auto value = in_message_.CFG_VALGET.cfgData;
                    ESP_LOGI(TAG, "%lld ", value);
                    break;
                }
                default:
                    ESP_LOGI(TAG, "unknown: %x", message_type_);
            }
            break;

        default:
            ESP_LOGI(TAG,"Unknown (%d-%d)\n", message_class_, message_type_);
            break;
    }

    m_listener.onUbxMsg(message_class_, message_type_, in_message_);

    return true;
}

