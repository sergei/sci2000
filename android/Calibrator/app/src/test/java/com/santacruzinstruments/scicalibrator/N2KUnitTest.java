package com.santacruzinstruments.scicalibrator;

import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.SciWindCalibration_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.windData_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.Utils.Utils.radstodegs;
import static com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000.MHU_CALIBRATION_PGN;
import static com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000.makeGroupCommandPacket;
import static com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000.makeGroupRequestPacket;

import org.junit.BeforeClass;
import org.junit.Test;

import static org.junit.Assert.*;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.santacruzinstruments.scicalibrator.nmea2000.CanFrameAssembler;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KField;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KLib;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacket;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KTypeException;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K;
import com.santacruzinstruments.scicalibrator.nmea2000.can.TransportTask;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.LinkedList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

import timber.log.Timber;

public class N2KUnitTest {

    @BeforeClass
    public static void SetupTimber(){
        Timber.plant(new Timber.Tree() {
            @Override
            protected void log(int priority, @Nullable String tag, @NonNull String message, @Nullable Throwable t) {
                System.out.println(message);
            }
        });
    }

    @Test
    public void decodingTest() throws IOException {
        InputStream is = Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json");
        new N2KLib(null, is);
        AtomicBoolean twaDecoded = new AtomicBoolean(false);
        AtomicBoolean twsDecoded = new AtomicBoolean(false);
        CanFrameAssembler canFrameAssembler = new CanFrameAssembler((pgn, priority, dest, src, time, rawBytes, len, hdrlen) -> {
            N2KPacket packet = new N2KPacket(pgn, priority, dest, src, time, rawBytes, len, hdrlen);
            assertTrue(packet.isValid() || packet.pgn == 126998); // PGN 126998 not has supported field type of LENCTRLSTRING
            Timber.d("pkt %s\n", packet);

            if (packet.pgn == windData_pgn) {
                try {
                    if (packet.fields[N2K.windData.windSpeed].getAvailability() == N2KField.Availability.AVAILABLE) {
                        double tws = packet.fields[N2K.windData.windSpeed].getDecimal();
                        twsDecoded.set(true);
                        System.out.printf("TWS=%.1f\n", tws);
                    }
                    if (packet.fields[N2K.windData.windAngle].getAvailability() == N2KField.Availability.AVAILABLE) {
                        double twa = radstodegs(packet.fields[N2K.windData.windAngle].getDecimal());
                        twaDecoded.set(true);
                        System.out.printf("TWA=%.1f\n", twa);
                    }
                } catch (N2KTypeException e) {
                    e.printStackTrace();
                }
            }
        });

        BufferedReader reader = new BufferedReader(new InputStreamReader(Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("raw.txt")));
        while (reader.ready()) {
            String s = reader.readLine();
            parseString(canFrameAssembler, s);
        }

        assertTrue(twaDecoded.get());
        assertFalse(twsDecoded.get());
    }

    private void parseString(CanFrameAssembler canFrameAssembler, String s) {
        String[] t = s.split(" ");
        if (t.length > 2) {
            if (Objects.equals(t[1], "R")) {
                int canAddr = Integer.parseInt(t[2], 16);
                byte[] data = new byte[t.length - 3];
                for (int i = 0; i < data.length; i++) {
                    data[i] = (byte) Integer.parseInt(t[i + 3], 16);
                }
                canFrameAssembler.setFrame(canAddr, data);
            }
        }
    }

    @Test
    public void encodingTest() {

        final CanFrameAssembler canFrameAssembler = new CanFrameAssembler((pgn, priority, dest, src, time, rawBytes, len, hdrlen) -> {
        });

        // Need to create N2KLib() so some internal statics are initialized
        new N2KLib(null, Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json"));

        N2KPacket p = makeGroupRequestPacket(MHU_CALIBRATION_PGN);

        assertNotNull(p);

        List<byte[]> frames = canFrameAssembler.makeCanFrames(p);

        String [] expectedReq = {
                "09EDFF00 40 10 00 54 FF 01 FF FF\r\n",
                "09EDFF00 41 FF FF FF FF 02 01 E4\r\n",
                "09EDFF00 42 07 03 04 FF FF FF FF\r\n"
        };

        assertEquals(expectedReq.length, frames.size());

        List<String> generated = new LinkedList<>();
        for( byte[] data : frames){
            String msg = TransportTask.formatYdnuRawString(canFrameAssembler.getCanAddr(), data);
            generated.add(msg);
        }

        for( int i = 0; i < expectedReq.length; i++){
            assertEquals(expectedReq[i], generated.get(i));
        }

        p = makeGroupCommandPacket(MHU_CALIBRATION_PGN, (byte)3, 4, 2);
        String [] expectedCmd = {
                "09ED0300 40 16 00 54 FF 01 FF FF\r\n",
                "09ED0300 41 FF FF FF FF 04 01 E4\r\n",
                "09ED0300 42 07 03 04 04 FE FF 05\r\n",
                "09ED0300 43 02 00 FF FF FF FF FF\r\n"
        };

        assertNotNull(p);

        frames = canFrameAssembler.makeCanFrames(p);
        assertEquals(expectedCmd.length, frames.size());

        generated = new LinkedList<>();
        for( byte[] data : frames){
            String msg = TransportTask.formatYdnuRawString(canFrameAssembler.getCanAddr(), data);
            generated.add(msg);
        }

        for( int i = 0; i < expectedCmd.length; i++){
            assertEquals(expectedCmd[i], generated.get(i));
        }

    }

    @Test
    public void calDecodingTest(){
        Timber.d("Test");

        // Need to create N2KLib() so some internal statics are initialized
        new N2KLib(null, Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json"));
        CanFrameAssembler canFrameAssembler = new CanFrameAssembler((pgn, priority, dest, src, time, rawBytes, len, hdrlen) -> {
            N2KPacket packet = new N2KPacket(pgn, priority, dest, src, time, rawBytes, len, hdrlen);
            assertTrue(packet.isValid());
            Timber.d("pkt %s\n", packet);

            if (packet.pgn == SciWindCalibration_pgn) {
                if ( packet.fields[N2K.SciWindCalibration.AWSMultiplier].getAvailability() == N2KField.Availability.AVAILABLE ){
                    try {
                        double awsCal = packet.fields[N2K.SciWindCalibration.AWSMultiplier].getDecimal();
                        assertEquals(10.81, awsCal, 0.001);
                    } catch (N2KTypeException e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        /*
I (323410) mhu2nmea_CalibrationStorage: Opening Non-Volatile Storage (NVS) handle...
I (323420) mhu2nmea_CalibrationStorage: Read AWA calibration 134
I (323420) mhu2nmea_CalibrationStorage: Opening Non-Volatile Storage (NVS) handle...
I (323430) mhu2nmea_CalibrationStorage: Read AWS calibration 1081
I (323440) mhu2nmea_ESP32N2kStream: 323111: Send PGN:130900323111:  - can ID:16772814320,6,9c,fc,86,0,39,4
I (323450) mhu2nmea_ESP32N2kStream: 323121 : Pri:2 PGN:130900 Source:15 Dest:255 Len:6 Data:9c,fc,86,0,39,4

         */
        String msg = "00:06:00.118 R 09FF540F C0 06 9C FC 86 00 39 04";
        parseString(canFrameAssembler, msg);


    }

}