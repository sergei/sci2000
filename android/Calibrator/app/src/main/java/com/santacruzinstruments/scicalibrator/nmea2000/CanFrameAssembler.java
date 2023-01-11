package com.santacruzinstruments.scicalibrator.nmea2000;

import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacket;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class CanFrameAssembler {

    private int  n2k_to_can_id(byte priority, int pgn, byte src, byte dst) {
        int can = (pgn >> 8) & 0x00FF;

        if (can< 240) {  // #PDU1 format
            if ((pgn & 0xff) != 0)
                return 0; //  #for PDU1 format PGN lowest byte has to be 0 for the destination.
            return ((priority & 0x7) << 26) | (pgn << 8) | ((dst & 0x00FF) << 8) | (src & 0x00FF);
        }
        else { //  #PDU2 format
            return ((priority & 0x7) << 26) | (pgn << 8) | (src & 0x00FF);
        }
    }

    int canAddr = 0;
    byte [] rawData = new byte[233]; // Max size of NMEA fast packet
    public List<byte[]> makeCanFrames(N2KPacket p) {
        canAddr = n2k_to_can_id(p.priority, p.pgn, p.src, p.dest);
        int dl = p.getRawData(rawData, 0);
        ArrayList<byte[]> frames = new ArrayList<>();
        if ( dl <= 8 ){
            frames.add(Arrays.copyOfRange(rawData,0, dl));
        }else{
            int n_pad = 7 - ((dl + 1) % 7) % 7;
            for( int i = dl; i < dl+n_pad; i++)
                rawData[i] = (byte) 0xFF;
            int len = (dl + 1) + n_pad;
            int nframes = len / 7;
            byte seq = (byte) 0x40;
            int rawDataIdx = 0;
            for(int fr = 0; fr < nframes; fr++){
                byte [] data = new byte[8];
                data[0] = (byte) (seq | fr);
                int startIdx = 1;
                if ( fr == 0 ){
                    data[1] = (byte) dl;
                    startIdx = 2;
                }
                for( int i = startIdx; i < 8; i++){
                    data[i] = rawData[rawDataIdx++];
                }
                frames.add(data);
            }
        }
        return frames;
    }

    public int getCanAddr() {
        return canAddr;
    }

    public interface N2kListener {
        void onN2kPacket(int pgn, byte priority, byte dest, byte src, int time,  byte[] rawBytes, int len, int hdrlen);
    }
    private final N2kListener listener;
    public CanFrameAssembler(N2kListener listener){
        this.listener = listener;

    }
    public void setFrame(int can_id, byte[] data){
        int can_id_pf = (can_id >> 16) & 0x00FF;
        byte can_id_ps = (byte) ((can_id >> 8) & 0x00FF);
        int can_id_dp = (can_id >> 24) & 1;

        byte src = (byte) (can_id  & 0x00FF);
        byte priority = (byte) ((can_id >> 26) & 0x7);

        byte dst;
        int pgn;

        if (can_id_pf < 240){
            /* PDU1 format, the PS contains the destination address */
            dst = can_id_ps;
            pgn = (can_id_dp << 16) | ((can_id_pf) << 8);
        }
        else{
            /* PDU2 format, the destination is implied global and the PGN is extended */
            dst = (byte) 0xff;
            pgn = (can_id_dp << 16) | (can_id_pf << 8) |can_id_ps;
        }

        this.listener.onN2kPacket(pgn, priority, dst, src, 0, data, data.length,0);
    }


}
