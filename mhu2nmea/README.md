# ESP32 project for MHU analog sensors to N2K gateway

The CLion instructions are taken from this [page](https://www.jetbrains.com/help/clion/esp-idf.html)
## Hardware 

* The [Sailor HAT](https://docs.hatlabs.fi/sh-esp32/) board is used to run this SW
* The [MHU analog outputs](doc/B&G FAQ-MHU Wiring and Tests.pdf) are connected to [ADC](https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/adc.html) channels of ESP32
  * Wind Angle Phase (Red) - ADC2_CHANNEL_5 - GPIO 12 
  * Wind Angle Phase (Green) - ADC2_CHANNEL_6 - GPIO 14
  * Wind Angle Phase (Blue) - ADC2_CHANNEL_3 - GPIO 15
  * Wind Speed Signal (Violet) - ADC2_CHANNEL_8 - GPIO 25
Wind Angle phase is encoded by the voltage in the range 0 - 6.4 Volts, the ESP32 ADC range is 0 - 3.3 V  
