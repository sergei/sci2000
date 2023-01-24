# B&G NET Protocol reverse engineering
This folder contains attempt to reverse engineer B&G Net protocol

First I performed the captures on NET wire (pin 3) with a scope located in [this folder](net-captures/oscilloscope)
it appears to be the UART protocol 9600,N,1 with levels 0 - 5V it has the opposite polarity  to NMEA output present on Pin 4
In order to read it with RS-232 dongle the adapter with MAX chip was used 

Once the serial data was established I performed several captures with the different values of AWA and AWS. 
The mhu_sim was used as the input to B&G Network WIND instrument. The serial captures are located in [this folder](net-captures/rs-232)

There is some structure to the serial data, not quite sure I quite understand it yet.
Here are packets I was able to discern:
<pre>
AWS 15 AWA 0  | ff 30 84 01  3c  c5 a0 80 b8  a0 c0 85 81  ae c0 85 80  21 01 80 04  22 00 02 e8  8a
AWS 15 AWA 0  | ff 30 84 01  c4                            a6 c0 04 7d  21 01 80 04  22 00 02 e8  21
AWS 10 AWA 0  | ff 30 84 01  3c  cd a0 80 b8  a0 c8 83 c1  a6 c0 83 40  21 01 80 04  22 00 02 e8  8e
AWS 10 AWA 0  | ff 30 84 01  c4                            a6 c8 83 b8  21 01 80 04  22 00 02 e8  e7
AWS 5 AWA 0   | ff 30 84 01  3c  c5 a0 80 b8  a0 c0 01 78  a6 c0 01 78  21 01 80 04  22 00 02 e8  93
AWS 5 AWA 0   | ff 30 84 01  c4                            a6 c0 01 72  21 01 80 04  22 00 02 e8  27
AWS 0 AWA 0   | ff 30 84 01  3c  c5 a0 80 b8  a0 c0 80 80  a6 c0 80 80  21 01 80 04  22 00 02 e8  15
AWS 0 AWA 0   | ff 30 84 01  c4                            a6 c0 80 80  21 01 80 04  22 00 02 e8  22
AWS 0 AWA 45  | ff 30 84 01  3c  c5 a8 80 39  a0 c8 80 80  a6 c0 80 80  21 01 80 11  22 00 92 10  47
AWS 0 AWA 45  | ff 30 84 01  c4                            a6 c0 80 80  21 01 80 11  22 08 92 10  55
AWS 0 AWA 90  | ff 30 84 01  3c  c5 a8 80 39  a0 c0 80 80  a6 c0 80 80  21 01 80 2e  22 08 a2 e4  c6
AWS 0 AWA 90  | ff 30 84 01  c4                            ae c0 80 80  21 01 80 2e  22 00 a2 e4  d4
AWS 0 AWA 135 | ff 30 84 01  3c  c5 a0 80 39  a0 c0 80 80  a6 c0 80 80  21 01 80 c3  22 00 32 e2  23
AWS 0 AWA 135 | ff 30 84 01  c4                            a6 c0 80 80  21 01 80 c3  22 00 32 e2  b9
AWS 0 AWA 180 | ff 30 84 01  3c  c5 a8 80 b8  a0 c8 80 80  a6 c8 80 80  21 01 ff a0  22 08 c2 60  32
AWS 0 AWA 180 | ff 30 84 01  c4                            a6 c0 80 80  21 01 ff a0  22 00 c2 60  c7
AWS 0 AWA 270 | ff 30 84 01  3c  c5 a0 80 b8  a0 c0 80 80  a6 c0 80 80  21 01 ff 5b  22 00 62 06  c1
AWS 0 AWA 270 | ff 30 84 01  c4                            a6 c0 80 80  21 01 ff 4b  22 00 62 06  d6
</pre>

Maybe the serial data is not too clean, and I was not bale to determine if the last byte is the checksum 

Most likely 22 00 is followed by two bytes of AWA and a6 c0 is followed by two bytes of AWS.  






