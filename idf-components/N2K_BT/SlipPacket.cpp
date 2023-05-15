#include <esp_log.h>
#include "SlipPacket.h"

void SlipPacket::onSlipByteReceived(unsigned char byte) {
    if (byte == SLIP_END) {
        if (slipBufferIndex > 0) {
            listener.onPacketReceived(slipBuffer, slipBufferIndex);
        }
        slipBufferIndex = 0;
    } else if (byte == SLIP_ESC) {
        escapeNext = true;
    } else {
        if (escapeNext) {
            escapeNext = false;
            if (byte == SLIP_ESC_END) {
                byte = SLIP_END;
            } else if (byte == SLIP_ESC_ESC) {
                byte = SLIP_ESC;
            }
        }
        if (slipBufferIndex < sizeof(slipBuffer)) {
            slipBuffer[slipBufferIndex++] = byte;
        }
    }
}

void SlipPacket::EncodeAndSendPacket(const unsigned char *inputBuffer, unsigned char len) {
    unsigned char outputBuffer[OUT_BUF_LEN];
    int outIdx = 0;

    for(int i=0; i < len; i++) {
        if( outIdx >= OUT_BUF_LEN - 4 ) {
            ESP_LOGE("SlipPacket", "EncodeAndSendPacket: outBuf overflow");
            return;
        }

        if(inputBuffer[i] == SLIP_END ) {
            outputBuffer[outIdx++] = SLIP_ESC;
            outputBuffer[outIdx++] = SLIP_ESC_END;
        } else if(inputBuffer[i] == SLIP_ESC ) {
            outputBuffer[outIdx++] = SLIP_ESC;
            outputBuffer[outIdx++] = SLIP_ESC_ESC;
        } else {
            outputBuffer[outIdx++] = inputBuffer[i];
        }
    }
    outputBuffer[outIdx++] = SLIP_END;
    outStream.sendEncodedBytes(outputBuffer, outIdx);
}
