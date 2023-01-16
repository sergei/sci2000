package com.santacruzinstruments.scicalibrator.nmea2000;

import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.SciImuCalibration_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.SciWaterCalibration_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.SciWindCalibration_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.attitude_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.nmeaRequestGroupFunction_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.speed_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.vesselHeading_pgn;
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

import timber.log.Timber;

public class Nmea2000 implements TransportTask.UsbConnectionListener {

    public interface N2KListener {
        void OnConnectionStatus(boolean connected);
        void onRcvdValue(ItemType item, double value);
        void onRcvdCalibration(ItemType item, double cal);

    }

    public final static int SCI_MFG_CODE = 2020;  // # Our mfg code.
    public final static int SCI_INDUSTRY_CODE = 4;  // Marine industry

    private static Nmea2000 instance = null;

    private final TransportTask transportTask;
    private final CanFrameAssembler canFrameAssembler;
    private final N2KListener listener;
    private boolean isConnected = false;

    boolean mhuCalReceived = false;
    private byte mhuCalDest = 0;  // Address of device where send the MHU calibration

    boolean speedCalReceived = false;
    private byte speedCalDest = 0;  // Address of device where to send the paddle wheel calibration

    boolean imuCalReceived = false;
    private byte imuCalDest = 0;  // Address of device where to send IMU calibration

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
            if ( packet.isValid() ){
                try {
                    processIncomingPacket(packet);
                } catch (N2KTypeException e) {
                    Timber.e(e,"Failed to parse packet");
                }
            }
        });
    }

    private void processIncomingPacket(N2KPacket packet) throws N2KTypeException {
        switch(packet.pgn){
            case windData_pgn:
                if ( packet.fields[N2K.windData.windSpeed].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double aws = packet.fields[N2K.windData.windSpeed].getDecimal();
                    this.listener.onRcvdValue(ItemType.AWS, aws);
                }
                if ( packet.fields[N2K.windData.windAngle].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double awa = radstodegs(packet.fields[N2K.windData.windAngle].getDecimal());
                    this.listener.onRcvdValue(ItemType.AWA, awa);
                }
                break;
            case speed_pgn:
                if ( packet.fields[N2K.speed.speedWaterReferenced].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double spd = packet.fields[N2K.speed.speedWaterReferenced].getDecimal();
                    this.listener.onRcvdValue(ItemType.SPD, spd);
                }
                break;
            case vesselHeading_pgn:
                if ( packet.fields[N2K.vesselHeading.heading].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double hdg = packet.fields[N2K.vesselHeading.heading].getDecimal();
                    this.listener.onRcvdValue(ItemType.HDG, hdg);
                }
                break;
            case attitude_pgn:
                if ( packet.fields[N2K.attitude.pitch].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double pitch = packet.fields[N2K.attitude.pitch].getDecimal();
                    this.listener.onRcvdValue(ItemType.PITCH, pitch);
                }
                if ( packet.fields[N2K.attitude.roll].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double roll = packet.fields[N2K.attitude.roll].getDecimal();
                    this.listener.onRcvdValue(ItemType.ROLL, roll);
                }
                break;
            case SciWindCalibration_pgn:
                mhuCalReceived = true;
                this.mhuCalDest = packet.src;
                if ( packet.fields[N2K.SciWindCalibration.AWSMultiplier].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double awsCal = packet.fields[N2K.SciWindCalibration.AWSMultiplier].getDecimal();
                    this.listener.onRcvdCalibration(ItemType.AWS, awsCal);
                }
                if ( packet.fields[N2K.SciWindCalibration.AWAOffset].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double awaCal = packet.fields[N2K.SciWindCalibration.AWAOffset].getDecimal();
                    this.listener.onRcvdCalibration(ItemType.AWA, awaCal);
                }
                break;
            case SciWaterCalibration_pgn:
                speedCalReceived = true;
                this.speedCalDest = packet.src;
                if ( packet.fields[N2K.SciWaterCalibration.SOWMultiplier].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double speedCal = packet.fields[N2K.SciWaterCalibration.SOWMultiplier].getDecimal();
                    this.listener.onRcvdCalibration(ItemType.SPD, speedCal);
                }
                break;
            case SciImuCalibration_pgn:
                imuCalReceived = true;
                this.imuCalDest = packet.src;
                if ( packet.fields[N2K.SciImuCalibration.HeadingGOffset].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double cal = packet.fields[N2K.SciImuCalibration.HeadingGOffset].getDecimal();
                    this.listener.onRcvdCalibration(ItemType.HDG, cal);
                }
                if ( packet.fields[N2K.SciImuCalibration.PitchOffset].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double cal = packet.fields[N2K.SciImuCalibration.PitchOffset].getDecimal();
                    this.listener.onRcvdCalibration(ItemType.PITCH, cal);
                }
                if ( packet.fields[N2K.SciImuCalibration.RollOffset].getAvailability() == N2KField.Availability.AVAILABLE ){
                    double cal = packet.fields[N2K.SciImuCalibration.RollOffset].getDecimal();
                    this.listener.onRcvdCalibration(ItemType.ROLL, cal);
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
        this.isConnected = connected;
        listener.OnConnectionStatus(connected);
    }

    @Override
    public void onFrameReceived(int canAddr, byte[] data) {
        canFrameAssembler.setFrame(canAddr, data);
    }

    @Override
    public void onTick() {
        if ( this.isConnected ){

            if( !mhuCalReceived){
                Timber.d("Requesting MHU calibration");
                requestCurrentCal(SciWindCalibration_pgn);
            }

            if ( ! speedCalReceived ){
                Timber.d("Requesting SPEED calibration");
                requestCurrentCal(SciWaterCalibration_pgn);
            }

            if ( ! imuCalReceived ){
                Timber.d("Requesting IMU calibration");
                requestCurrentCal(SciImuCalibration_pgn);
            }
        }
    }

    public void sendCal(ItemType item, float calValue) {
        N2KPacket p = null;
        switch (item){
            case AWA:
                p = makeGroupCommandPacket(SciWindCalibration_pgn, this.mhuCalDest, N2K.SciWindCalibration.AWAOffset, calValue);
                mhuCalReceived = false;
                break;
            case AWS:
                p = makeGroupCommandPacket(SciWindCalibration_pgn, this.mhuCalDest, N2K.SciWindCalibration.AWSMultiplier, calValue);
                mhuCalReceived = false;
                break;
            case SPD:
                p = makeGroupCommandPacket(SciWaterCalibration_pgn, this.speedCalDest, N2K.SciWaterCalibration.SOWMultiplier, calValue);
                speedCalReceived = false;
                break;
            case HDG:
                p = makeGroupCommandPacket(SciImuCalibration_pgn, this.imuCalDest, N2K.SciImuCalibration.HeadingGOffset, calValue);
                imuCalReceived = false;
                break;
            case PITCH:
                p = makeGroupCommandPacket(SciImuCalibration_pgn, this.imuCalDest, N2K.SciImuCalibration.PitchOffset, calValue);
                imuCalReceived = false;
                break;
            case ROLL:
                p = makeGroupCommandPacket(SciImuCalibration_pgn, this.imuCalDest, N2K.SciImuCalibration.RollOffset, calValue);
                imuCalReceived = false;
                break;
        }

        if ( p != null) {
            sendPacket(p);
        }
    }

    private void sendPacket(N2KPacket p) {
        Timber.d("Sending %s", p.toString());
        List<byte[]> frames = canFrameAssembler.makeCanFrames(p);
        for(byte[] data: frames){
            transportTask.sendCanFrame(canFrameAssembler.getCanAddr(), data);
        }
    }

    static public N2KPacket makeGroupRequestPacket(int commandedPgn) {
        int[] functionCode = {0};  // Request
        N2KPacket p = new N2KPacket(nmeaRequestGroupFunction_pgn, functionCode);
        p.dest = (byte) 255; // Broadcast
        p.priority = 2;
        p.src = 0;
        try {
            p.fields[N2K.nmeaRequestGroupFunction.functionCode].setInt(functionCode[0]);
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

    static public N2KPacket makeGroupCommandPacket(int commandedPgn, byte dest, int fieldIdx, double value) {
        int[] functionCode = {1};  // Command
        N2KPacket p = new N2KPacket(nmeaRequestGroupFunction_pgn, functionCode);
        p.dest = dest;
        p.priority = 2;
        p.src = 0;
        try {
            p.fields[N2K.nmeaRequestGroupFunction.functionCode].setInt(functionCode[0]);
            p.fields[N2K.nmeaRequestGroupFunction.pgn].setInt(commandedPgn);
            p.fields[N2K.nmeaRequestGroupFunction.transmissionInterval].setInt(0xFFFFFFFF);
            p.fields[N2K.nmeaRequestGroupFunction.transmissionIntervalOffset].setInt(0xFFFF);
            p.fields[N2K.nmeaRequestGroupFunction.OfParameters].setInt(2 + 1);  // Always set only one parameter

            //
            N2KField[] repset = p.addRepSet();
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(1);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(16);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(SCI_MFG_CODE);
            repset = p.addRepSet();
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(3);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setBitLength(8);
            repset[N2K.nmeaRequestGroupFunction.rep.value].setInt(SCI_INDUSTRY_CODE);

            // Set parameter of interest
            repset = p.addRepSet();
            // Parameter field number
            repset[N2K.nmeaRequestGroupFunction.rep.parameter].setInt(fieldIdx + 1);
            // We need to know type and resolution of the field we are setting
            // Get the commanded PGN definition
            N2KPacket cmdPgn = new N2KPacket(commandedPgn);
            if ( cmdPgn.fields != null && fieldIdx < cmdPgn.fields.length ){
                final N2KField valueField = repset[N2K.nmeaRequestGroupFunction.rep.value];
                valueField.fieldDef = cmdPgn.fields[fieldIdx].fieldDef;  // Replace field definition with one from the commanded PGN
                valueField.setDecimal(value);
            }

            return p;
        } catch (N2KTypeException e) {
            e.printStackTrace();
            return null;
        }
    }

}
