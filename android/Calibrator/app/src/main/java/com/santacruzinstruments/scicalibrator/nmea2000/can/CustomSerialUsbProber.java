package com.santacruzinstruments.scicalibrator.nmea2000.can;

import com.hoho.android.usbserial.driver.CdcAcmSerialDriver;
import com.hoho.android.usbserial.driver.ProbeTable;
import com.hoho.android.usbserial.driver.UsbSerialProber;

/**
 * add devices here, that are not known to DefaultProber
 *
 * if the App should auto start for these devices, also
 * add IDs to app/src/main/res/xml/usb_device_filter.xml
 */
class CustomSerialUsbProber {

    static UsbSerialProber getCustomProber() {
        ProbeTable customTable = new ProbeTable();
        customTable.addProduct(1155, 41495, CdcAcmSerialDriver.class);   // (Yacht Devices Ltd.)

        return new UsbSerialProber(customTable);
    }

}
