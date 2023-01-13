package com.santacruzinstruments.scicalibrator.nmea2000.can;

import static com.santacruzinstruments.scicalibrator.BuildConfig.APPLICATION_ID;
import static java.lang.Thread.sleep;
import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;
import java.io.IOException;
import java.util.Objects;

import timber.log.Timber;

public class TransportTask implements SerialInputOutputManager.Listener {

    public interface UsbConnectionListener {
        void OnConnectionStatus(boolean connected);
        void onFrameReceived(int addrPri, byte[]  data);
        void onTick();
    }

    private boolean bKeepRunning = true;
    private UsbDeviceConnection usbConnection;
    private final UsbSerialProber usbDefaultProber = UsbSerialProber.getDefaultProber();
    private final UsbSerialProber usbCustomProber = CustomSerialUsbProber.getCustomProber();

    private enum Connected { False, Pending, True }
    private static final int WRITE_WAIT_MILLIS = 2000; // 0 blocked infinitely on unprogrammed arduino

    private static final String INTENT_ACTION_GRANT_USB = APPLICATION_ID + "INTENT_ACTION_GRANT_USB";
    private static final String INTENT_ACTION_DISCONNECT = APPLICATION_ID + "INTENT_ACTION_DISCONNECT";
    private final UsbConnectionListener usbConnectionListener;
    private final UsbManager usbManager;
    private UsbSerialPort usbSerialPort;
    private final PendingIntent usbPermissionIntent;
    private Connected connected = Connected.False;
    private final BroadcastReceiver disconnectBroadcastReceiver;
    private boolean gatewayInitialized = false;

    private SerialInputOutputManager ioManager;

    private UsbDevice usbDevice = null;
    boolean connectionEvent = false;

    private final Context context;

    private String rawString = "";

    public TransportTask(Context context, UsbConnectionListener usbConnectionListener) {
        this.context = context;
        this.usbConnectionListener = usbConnectionListener;
        usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        @SuppressLint("ObsoleteSdkInt") int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0;
        usbPermissionIntent = PendingIntent.getBroadcast(context, 0, new Intent(INTENT_ACTION_GRANT_USB), flags);

        BroadcastReceiver permissionGrantedBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                Timber.d("Received action %s", intent.getAction());
                if (INTENT_ACTION_GRANT_USB.equals(intent.getAction())) {
                    Boolean granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false);
                    connect(granted);
                }
            }
        };
        context.registerReceiver(permissionGrantedBroadcastReceiver, new IntentFilter(INTENT_ACTION_GRANT_USB));

        disconnectBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                disconnect(); // disconnect now, else would be queued until UI re-attached
            }
        };
    }

    // Called by activity when USB attachment caused us to start
    // Or when polling by checkConnectedDevices()
    public synchronized void setUsbDevice(UsbDevice device) {
        this.usbDevice = device;
        connectionEvent = true;
    }


    // Called periodically to see if the device was connected while we are running
    private void checkConnectedDevices() {
        for (UsbDevice device : usbManager.getDeviceList().values()) {
            UsbSerialDriver driver = usbDefaultProber.probeDevice(device);
            if(driver == null) {
                driver = usbCustomProber.probeDevice(device);
            }
            if(driver != null) {
                setUsbDevice(device);
            }
        }
    }

    public synchronized void stop(){
        bKeepRunning = false;
    }

    @SuppressWarnings("BusyWait")
    public void run() {
        Timber.d("Starting Serial USB Manager thread");
        while (bKeepRunning) {

            if (connectionEvent){
                connectionEvent = false;
                connect(null);
            }else if ( connected == Connected.False ){
                checkConnectedDevices();
            }

            if ( connected == Connected.True){
                if ( ! this.gatewayInitialized){
                    // Set gateway to 0183 mode, so it would translate from NMEA 2000 to NMEA 0183
                    byte [] init = "YDNU MODE RAW\r\n".getBytes();
                    try {
                        write(init);
                        this.gatewayInitialized = true;
                    } catch (IOException e) {
                        Timber.e(e, "Could not write to YDNU");
                    }
                }
            }
            usbConnectionListener.onTick();
            try {sleep(1000);} catch (InterruptedException ignore) {}
        }
    }

    private void connect(Boolean permissionGranted) {
        Timber.d("Connecting with %s permissions", permissionGranted == null ? "no" : "");
        UsbSerialDriver driver = UsbSerialProber.getDefaultProber().probeDevice(this.usbDevice);
        if(driver == null) {
            driver = CustomSerialUsbProber.getCustomProber().probeDevice(this.usbDevice);
        }
        if(driver == null) {
            Timber.e("connection failed: no driver for device");
            return;
        }
        if(driver.getPorts().size() < 1) {
            Timber.e("connection failed: not enough ports at device");
            return;
        }
        usbSerialPort = driver.getPorts().get(0);
        usbConnection = usbManager.openDevice(driver.getDevice());
        if(usbConnection == null && permissionGranted == null && !usbManager.hasPermission(driver.getDevice())) {
            Timber.d("Requesting permission");
            usbManager.requestPermission(driver.getDevice(), usbPermissionIntent);
            return;
        }

        if(usbConnection == null) {
            if (!usbManager.hasPermission(driver.getDevice()))
                Timber.e("connection failed: permission denied");
            else
                Timber.e("connection failed: open failed");
            return;
        }

        connected = Connected.Pending;
        try {
            Timber.d("Opening port ...");
            usbSerialPort.open(usbConnection);
            usbSerialPort.setParameters(115200, UsbSerialPort.DATABITS_8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);

            context.registerReceiver(disconnectBroadcastReceiver, new IntentFilter(INTENT_ACTION_DISCONNECT));
            usbSerialPort.setDTR(true); // for arduino, ...
            usbSerialPort.setRTS(true);
            ioManager = new SerialInputOutputManager(usbSerialPort, this);
            ioManager.start();
            usbConnectionListener.OnConnectionStatus(true);
            connected = Connected.True;
        } catch (Exception e) {
            Timber.e(e, "Failed to open port");
            usbConnectionListener.OnConnectionStatus(false);
        }

    }

    private void disconnect() {
        Timber.d("Starting to disconnect");
        if (ioManager != null) {
            ioManager.setListener(null);
            ioManager.stop();
            ioManager = null;
        }
        if (usbSerialPort != null) {
            try {
                usbSerialPort.setDTR(false);
                usbSerialPort.setRTS(false);
            } catch (Exception ignored) {
            }
            try {
                usbSerialPort.close();
            } catch (Exception ignored) {
            }
            usbSerialPort = null;
        }

        if(usbConnection != null) {
            usbConnection.close();
            usbConnection = null;
        }
        try {
            this.context.unregisterReceiver(disconnectBroadcastReceiver);
        } catch (Exception ignored) {
        }
        connected = Connected.False;
        usbConnectionListener.OnConnectionStatus(false);
    }

    void write(byte[] data) throws IOException {
        if(usbSerialPort == null)
            throw new IOException("not connected");
        usbSerialPort.write(data, WRITE_WAIT_MILLIS);
    }

    @Override
    public void onNewData(byte[] data) {
        for( byte b: data){
            if ( b == '\n' || b == '\r'){
                if ( rawString.length() > 0) {
                    processRawString(rawString);
                    rawString = "";
                }
            }else{
                rawString += (char)(b);
            }
        }

    }

    private void processRawString(String s) {
        String [] t = s.split(" ");
        Timber.d("Got RAW string [%s] (%d)", s, t.length);
        if( t.length > 2 && t.length <= 11) {
            if(Objects.equals(t[1], "R")){
                try {
                    int canAddr = Integer.parseInt(t[2], 16);
                    byte [] data = new byte[t.length - 3];
                    for( int i=0; i < data.length; i++){
                        data[i] = (byte)Integer.parseInt(t[i + 3], 16);
                    }
                    usbConnectionListener.onFrameReceived(canAddr, data);
                }catch (NumberFormatException e){
                    Timber.w("Malformed raw string");
                }
            }
        }
    }

    static public String formatYdnuRawString(int canAddr, byte[] data){
        String msg = String.format("%08X", canAddr);
        for( byte b : data){
            msg += String.format(" %02X", b);
        }
        msg += "\r\n";
        return msg;
    }

    public void sendCanFrame(int canAddr, byte[] data){
        String msg = formatYdnuRawString(canAddr, data);
        try {
            write(msg.getBytes());
        } catch (IOException e) {
            Timber.e(e, "Failed to write");
            usbConnectionListener.OnConnectionStatus(false);
        }
    }

    @Override
    public void onRunError(Exception e) {
        Timber.w(e, "USB error");
        disconnect();
    }


}

