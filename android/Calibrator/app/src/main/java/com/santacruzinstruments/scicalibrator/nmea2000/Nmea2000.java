package com.santacruzinstruments.scicalibrator.nmea2000;

import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.SciWindCalibration_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.nmeaRequestGroupFunction_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.windData_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.Utils.Utils.radstodegs;

import android.content.Context;

import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KField;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KLib;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacket;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KTypeException;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K;
import com.santacruzinstruments.scicalibrator.nmea2000.can.TransportTask;

import java.io.InputStream;
import java.util.List;
import java.util.Objects;

public class Nmea2000 implements TransportTask.UsbConnectionListener {

    public interface N2KListener {
        void onRcvdAwa(double twaDeg);
        void onCurrAwaCal(double twaCalDeg);
        void OnConnectionStatus(boolean connected);
    }

    private static Nmea2000 instance = null;
    private final TransportTask transportTask;
    private final CanFrameAssembler canFrameAssembler;
    private final N2KListener listener;
    boolean calRequested = false;
    public static final int MHU_CALIBRATION_PGN = 130900;  // MHU calibration
    private static final int SPEED_CALIBRATION_PGN = 130901;  // MHU calibration

    private byte windCalDest = 0;  // Address of device where to send wind calibration

    public static int SCI_MFG_CODE = 2020;  // # Our mfg code.
    public static int SCI_INDUSTRY_CODE = 4;  // Marine industry


    public static Nmea2000 getNmea2000Instance(){
        return instance;
    }

    public static void Start(Context context, N2KListener listener){
        if ( instance == null ) {
            instance = new Nmea2000(context, listener);
            instance.startTransportThread();
        }
    }

    private Nmea2000(Context context, N2KListener listener){
        this.listener = listener;
        transportTask = new TransportTask(context, this);
        InputStream is = Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json");
        new N2KLib(null, is);
        canFrameAssembler = new CanFrameAssembler((pgn, priority, dest, src, time, rawBytes, len, hdrlen) -> {
            N2KPacket packet = new N2KPacket(pgn, priority, dest, src, time, rawBytes, len, hdrlen);
            try {
                processIncomingPacket(packet);
            } catch (N2KTypeException e) {
                e.printStackTrace();
            }
        });

    }

    private void processIncomingPacket(N2KPacket packet) throws N2KTypeException {
        switch(packet.pgn){
            case windData_pgn:
                if ( packet.fields[N2K.windData.windSpeed].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double tws = packet.fields[N2K.windData.windSpeed].getDecimal();
                }
                if ( packet.fields[N2K.windData.windAngle].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double awa = radstodegs(packet.fields[N2K.windData.windAngle].getDecimal());
                    this.listener.onRcvdAwa(awa);
                }
                break;
            case SciWindCalibration_pgn:
                this.windCalDest = packet.src;
                if ( packet.fields[N2K.SciWindCalibration.AWSMultiplier].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double awsCal = radstodegs(packet.fields[N2K.SciWindCalibration.AWSMultiplier].getDecimal());
                }
                if ( packet.fields[N2K.SciWindCalibration.AWAOffset].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double awaCal = packet.fields[N2K.SciWindCalibration.AWAOffset].getDecimal();
                    this.listener.onCurrAwaCal(awaCal);
                }
                break;
        }
    }

    private void requestCurrentCal(int commandedPgn) {
        N2KPacket p = makeGroupRequestPacket(commandedPgn);

        if ( p != null) {
            sendPacket(p);
        }
    }

    private void startTransportThread() {
        final Thread thread = new Thread(transportTask::run);
        thread.setName("Transport thread");
        thread.start();
    }

    @Override
    public void OnConnectionStatus(boolean connected) {
        listener.OnConnectionStatus(connected);
    }

    @Override
    public void onFrameReceived(int canAddr, byte[] data) {
        canFrameAssembler.setFrame(canAddr, data);
    }

    @Override
    public void onTick() {
        if( ! calRequested ){
            requestCurrentCal(MHU_CALIBRATION_PGN);
            requestCurrentCal(SPEED_CALIBRATION_PGN);
        }
    }

    public void sendAwaCal(int twaCal) {
        int [] params = {twaCal, 0xFFFF};
        N2KPacket p = makeGroupCommandPacket(MHU_CALIBRATION_PGN, this.windCalDest, params);

        if ( p != null) {
            sendPacket(p);
        }
    }

    private void sendPacket(N2KPacket p) {
        List<byte[]> frames = canFrameAssembler.makeCanFrames(p);
        for(byte[] data: frames){
            transportTask.sendCanFrame(canFrameAssembler.getCanAddr(), data);
        }
    }

    static public N2KPacket makeGroupRequestPacket(int commandedPgn) {
        int[] request = {0};
        N2KPacket p = new N2KPacket(nmeaRequestGroupFunction_pgn, request);
        p.dest = (byte) 255; // Broadcast
        p.priority = 2;
        p.src = 0;
        try {
            p.fields[N2K.nmeaRequestGroupFunction.functionCode].setInt(0);
            p.fields[N2K.nmeaRequestGroupFunction.pgn].setInt(commandedPgn);
            p.fields[N2K.nmeaRequestGroupFunction.transmissionInterval].setInt(0xFFFFFFFF);
            p.fields[N2K.nmeaRequestGroupFunction.transmissionIntervalOffset].setInt(0xFFFF);
            p.fields[N2K.nmeaRequestGroupFunction.OfParameters].setInt(2);
            N2KField[] repset = p.addRepSet();
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(1);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(16);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(SCI_MFG_CODE);
            repset = p.addRepSet();
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(3);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(8);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(SCI_INDUSTRY_CODE);
            return p;
        } catch (N2KTypeException e) {
            e.printStackTrace();
            return null;
        }
    }

    static public N2KPacket makeGroupCommandPacket(int commandedPgn, byte dest, int [] values) {
        int[] cmd = {0};
        N2KPacket p = new N2KPacket(nmeaRequestGroupFunction_pgn, cmd);
        p.dest = dest;
        p.priority = 2;
        p.src = 0;
        try {
            p.fields[N2K.nmeaRequestGroupFunction.functionCode].setInt(0);
            p.fields[N2K.nmeaRequestGroupFunction.pgn].setInt(commandedPgn);
            p.fields[N2K.nmeaRequestGroupFunction.transmissionInterval].setInt(0xFFFFFFFF);
            p.fields[N2K.nmeaRequestGroupFunction.transmissionIntervalOffset].setInt(0xFFFF);
            p.fields[N2K.nmeaRequestGroupFunction.OfParameters].setInt(2 + values.length);
            N2KField[] repset = p.addRepSet();
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(1);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(16);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(SCI_MFG_CODE);
            repset = p.addRepSet();
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(3);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(8);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(SCI_INDUSTRY_CODE);

            for( int i = 0; i < values.length; i++){
                repset = p.addRepSet();
                repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(4 + i);
                repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(16);
                repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(values[i]);
            }

            return p;
        } catch (N2KTypeException e) {
            e.printStackTrace();
            return null;
        }
    }

}
