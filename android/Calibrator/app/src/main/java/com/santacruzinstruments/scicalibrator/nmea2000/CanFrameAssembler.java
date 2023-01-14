package com.santacruzinstruments.scicalibrator.nmea2000;

import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KDefs.PGNInfo;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KLib;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacket;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacketDef;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import timber.log.Timber;

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
            int len = (dl + 1) + n_pad;
            int nframes = len / 7;

            for( int i = dl; i < dl+n_pad; i++)
                rawData[i] = (byte) 0xFF;
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

    private static class FastFrames {
        int frameCount = 0 ;
        int len = 0;
        int size = 0;
        int totalFramesNum = 0;
        int seqId = 0;
        final byte [] data = new byte[256];
    }

    private final HashMap<Byte, FastFrames> fastFramesMap = new HashMap<>();

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

        List<N2KPacketDef> packetDefs = N2KLib.pgnDefs.get(pgn);
        if ( packetDefs == null){
            Timber.d("Unknow PGN %d ", pgn);
            return;
        }

        N2KPacketDef pd = packetDefs.get(0);  // Grab the firts, since all of them should have the same type
        PGNInfo.PGNType pgnType = pd.pgnInfo.Type;

        if ( pgnType == PGNInfo.PGNType.Single ){
            this.listener.onN2kPacket(pgn, priority, dst, src, 0, data, data.length,0);
        }if ( pgnType == PGNInfo.PGNType.Fast ){
            int seqId = data[0] & 0xE0;
            int seqNum = data[0] & 0x1F;
            if( seqNum == 0 ){
                FastFrames f = new FastFrames();
                f.size = (int)data[1] & 0x00FF;
                f.seqId = seqId;
                int n_pad = (7 - ((f.size + 1) % 7) % 7) % 7;
                int len = (f.size + 1) + n_pad;
                f.totalFramesNum = len / 7;
                fastFramesMap.put(src, f);
                System.arraycopy(data, 2, f.data, 0, data.length - 2);
                f.len = data.length - 2;
                f.frameCount = 1;
            }else{
                final FastFrames f = fastFramesMap.get(src);
                if( f != null && f.seqId == seqId && f.frameCount == seqNum) { // Check if we got sequence right
                    System.arraycopy(data, 1, f.data, f.len, data.length - 1);
                    f.len += data.length - 1;
                    f.frameCount ++;
                }else{
                    Timber.d("Ignoring orphan frame");
                    fastFramesMap.remove(src);
                }
            }

            final FastFrames f = fastFramesMap.get(src);
            if( f != null && f.frameCount ==  f.totalFramesNum){
                this.listener.onN2kPacket(pgn, priority, dst, src, 0, f.data, f.size,0);
            }
        }
    }
}
