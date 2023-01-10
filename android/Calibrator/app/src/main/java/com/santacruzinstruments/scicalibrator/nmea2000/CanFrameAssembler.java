package com.santacruzinstruments.scicalibrator.nmea2000;

public class CanFrameAssembler {
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
