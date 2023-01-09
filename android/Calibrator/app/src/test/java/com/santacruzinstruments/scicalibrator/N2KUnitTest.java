package com.santacruzinstruments.scicalibrator;

import org.junit.Test;

import static org.junit.Assert.*;

import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KLib;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KPacket;
import com.santacruzinstruments.scicalibrator.nmea2000.N2KLib.N2KLib.N2KTransport;

import java.io.IOException;
import java.io.InputStream;
import java.util.Objects;

public class N2KUnitTest {

    class TestTransport implements N2KTransport{

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
    public void addition_isCorrect() {
        TestTransport transport = new TestTransport();

        InputStream is = Objects.requireNonNull(getClass().getClassLoader()).getResourceAsStream("pgns.json");


        N2KLib n2KLib = new N2KLib(transport, is);

        assertEquals(4, 2 + 2);
    }
}