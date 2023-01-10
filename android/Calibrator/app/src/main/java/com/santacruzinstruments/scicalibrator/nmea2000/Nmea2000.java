package com.santacruzinstruments.scicalibrator.nmea2000;

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
import java.util.Objects;

public class Nmea2000 implements TransportTask.UsbConnectionListener {

    public interface N2KListener {
        void onTwa(double twaDeg);
        void onTwaCal(double twaCalDeg);
    }

    private static Nmea2000 instance = null;
    private final TransportTask transportTask;
    private final CanFrameAssembler canFrameAssembler;

    public static void Start(Context context, N2KListener listener){
        if ( instance == null ) {
            instance = new Nmea2000(context, listener);
            instance.startTransportThread();
        }
    }

    private Nmea2000(Context context, N2KListener listener){
        transportTask = new TransportTask(context, this);
        InputStream is = Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json");
        new N2KLib(null, is);
        canFrameAssembler = new CanFrameAssembler((pgn, priority, dest, src, time, rawBytes, len, hdrlen) -> {
            N2KPacket packet = new N2KPacket(pgn, priority, dest, src, time, rawBytes, len, hdrlen);
            if (packet.pgn == windData_pgn) {
                try {
                    if ( packet.fields[N2K.windData.windSpeed].getAvailability() == N2KField.Availability.AVAILABLE ){
                        double tws = packet.fields[N2K.windData.windSpeed].getDecimal();
                    }
                    if ( packet.fields[N2K.windData.windAngle].getAvailability() == N2KField.Availability.AVAILABLE ){
                        double twa = radstodegs(packet.fields[N2K.windData.windAngle].getDecimal());
                        listener.onTwa(twa);
                    }
                } catch (N2KTypeException e) {
                    e.printStackTrace();
                }
            }
        });

    }

    private void startTransportThread() {
        final Thread thread = new Thread(transportTask::run);
        thread.setName("Transport thread");
        thread.start();
    }

    @Override
    public void OnConnectionStatus(boolean connected) {

    }

    @Override
    public void onFrameReceived(int canAddr, byte[] data) {
        canFrameAssembler.setFrame(canAddr, data);
    }

}
