package com.santacruzinstruments.scicalibrator.nmea2000;

import android.content.Context;

import com.santacruzinstruments.scicalibrator.nmea2000.can.TransportTask;

import timber.log.Timber;

public class Nmea2000 implements TransportTask.UsbConnectionListener {

    public interface N2KListener {
        void onTwa(double twaDeg);
        void onTwaCal(double twaCalDeg);
    }

    private static Nmea2000 instance = null;
    private final TransportTask transportTask;
    private final N2KListener listener;

    public static void Start(Context context, N2KListener listener){
        if ( instance == null ) {
            instance = new Nmea2000(context, listener);
            instance.startTransportThread();
        }
    }

    private Nmea2000(Context context, N2KListener listener){
        this.listener = listener;
        transportTask = new TransportTask(context, this);
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
    public void onDataReceived(byte[] buff, int size) {
        Timber.d("Got %d bytes", size);
        listener.onTwa(0);
    }
}
