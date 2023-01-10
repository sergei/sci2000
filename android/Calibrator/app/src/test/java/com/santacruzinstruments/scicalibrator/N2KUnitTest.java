package com.santacruzinstruments.scicalibrator;

import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K.windData_pgn;
import static com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.Utils.Utils.radstodegs;

import org.junit.Test;

import static org.junit.Assert.*;

import com.santacruzinstruments.scicalibrator.nmea2000.CanFrameAssembler;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KField;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KLib;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacket;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KTransport;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KTypeException;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KMsgs.N2K;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

public class N2KUnitTest {

    static class TestTransport implements N2KTransport{

        @Override
        public void open() throws IOException {

        }

        @Override
        public void close() {

        }

        @Override
        public N2KPacket get() throws IOException {
            return null;
        }

        @Override
        public int put(N2KPacket packet) throws IOException {
            return 0;
        }
    }

    @Test
    public void decodingTest() throws IOException {
        TestTransport transport = new TestTransport();
        InputStream is = Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json");
        new N2KLib(transport, is);
        AtomicBoolean twaDecoded = new AtomicBoolean(false);
        AtomicBoolean twsDecoded = new AtomicBoolean(false);
        CanFrameAssembler canFrameAssembler = new CanFrameAssembler((pgn, priority, dest, src, time, rawBytes, len, hdrlen) -> {
            N2KPacket packet = new N2KPacket(pgn, priority, dest, src, time, rawBytes, len, hdrlen);
            System.out.printf("pkt %s\n", packet);

            if (packet.pgn == windData_pgn) {
                try {
                    if ( packet.fields[N2K.windData.windSpeed].getAvailability() == N2KField.Availability.AVAILABLE ){
                        double tws = packet.fields[N2K.windData.windSpeed].getDecimal();
                        twsDecoded.set(true);
                        System.out.printf("TWS=%.1f\n", tws);
                    }
                    if ( packet.fields[N2K.windData.windAngle].getAvailability() == N2KField.Availability.AVAILABLE ){
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
        while(reader.ready()) {
            String s = reader.readLine();
            String [] t = s.split(" ");
            if( t.length > 2) {
                if(Objects.equals(t[1], "R")){
                    int canAddr = Integer.parseInt(t[2], 16);
                    byte [] data = new byte[t.length - 3];
                    for( int i=0; i < data.length; i++){
                        data[i] = (byte)Integer.parseInt(t[i + 3], 16);
                    }
                    canFrameAssembler.setFrame(canAddr, data);
                }
            }
        }

        assertTrue(twaDecoded.get());
        assertFalse(twsDecoded.get());
    }
}