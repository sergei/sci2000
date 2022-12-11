# ESP32 project for MHU analog sensors to N2K gateway

The CLion instructions are taken from this [page](https://www.jetbrains.com/help/clion/esp-idf.html)
 * To build the image select "app" in configuration drop box and Click build  Cmd-F9  
 * To flash the image select "flash" in configuration drop box and Click build  Cmd-F9  
 * To monitor serial output the image select "monitor" in configuration drop box and Click build  Cmd-F9
   * To select the serial port go to CLion->Preferences (Cmd-,) then CMake Environment and put there ESPPORT=/dev/tty.usbserial-14130 or whatever serial port you have 
## Hardware 

* The [Sailor HAT](https://docs.hatlabs.fi/sh-esp32/) board is used to run this SW
* The [MHU analog outputs](doc/B&G FAQ-MHU Wiring and Tests.pdf) is connected to the following pins
  * Wind Angle Phase (Red) - Analog input A  of [Engine to Hat](https://docs.hatlabs.fi/sh-esp32/pages/add-ons/engine-hat/)  - CH0 of AD115 ADC
  * Wind Angle Phase (Green) - Analog input B of [Engine to Hat](https://docs.hatlabs.fi/sh-esp32/pages/add-ons/engine-hat/) - CH1 of AD115 ADC
  * Wind Angle Phase (Blue) - Analog input C of [Engine to Hat](https://docs.hatlabs.fi/sh-esp32/pages/add-ons/engine-hat/)  - CH2 of AD115 ADC
  * Wind Speed Signal (Violet) -  OPTO_IN (GPIO 35)
      
## Software 
### The AWA 
  The AWA inputs are connected to  [AD115 ADC](doc/ads1115.pdf) the I2C address is set to 0x4b (default on Top Hat)
  The software to read this input is implemented by the class [AWAHandler](main/AWAHandler.h) it has its own task and polls ADC periodically
  The algorithm to decode angle from the voltages on three wires is prototyped in this (script)[scripts/awa_decoder.py]
  The algorithm is not sensitive to the absolute values of the voltages, since their sum is supposed to be constant.
  The ADC doesn't allow to latch all three inputs simultaneously, so change of angle between readings introduces some error.
### The AWS 
  To read the AWS value we use the pulse counter feature of ESP32 the code is implemented in (CNTHandler)[main/CNTHandler.h] class.
  The interrupt is set to generate for every four pulses, that should increase the accuracy I hope. 
### NMEA 2000
  The NMEA 2000 sender is done in the (N2KHandler)[main/N2KHandler.h] class. It has its own task where it sends the wind PGN periodically

All classes communicate through the queue. The queue is polled in the (main)[main/mhu2nmea_main.cpp] function and data dispatched from there. 
