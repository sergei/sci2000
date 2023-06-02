# ESP32 project for Android Open Accessory to N2K gateway

## Hardware 

* The [Sailor HAT](https://docs.hatlabs.fi/sh-esp32/) board is used to run this SW

* [FTDI FT311 Full speed USB Host controller for Android.](https://ftdichip.com/products/ft311d-32l/) 

* FT311D board is connected to the ESP32 UART module using following pins
  * RX - GPIO 26 of ESP32
  * TX - GPIO 27 of ESP32
  * VCC - 5V of 
  * GND - ESP32 GND

## Software

### How to build and flash
The CLion instructions are taken from this [page](https://www.jetbrains.com/help/clion/esp-idf.html)
* To build the image select "app" in configuration drop box and Click build  Cmd-F9
* To flash the image select "flash" in configuration drop box and Click build  Cmd-F9
* To monitor serial output the image select "monitor" in configuration drop box and Click build  Cmd-F9
   * To select the serial port go to CLion->Preferences (Cmd-,) then CMake Environment and put there ESPPORT=/dev/tty.usbserial-14130 or whatever serial port you have

### NMEA 2000
  The NMEA 2000 sender is done in the (N2KHandler)[main/N2KHandler.h] class. It has its own task where it sends the wind PGN periodically

All classes communicate through the queue. The queue is polled in the (main)[main/imu2nmea_main.cpp] function and data dispatched from there. 
