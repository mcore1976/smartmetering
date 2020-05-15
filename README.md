# smartmetering
Smart metering of temperature and humidity using GSM/GPRS network or SMS text messages.

This is very simple example how to build IoT device that sends remote reading of temperature and humidity ( from DHT22 / DHT11 sensor) with text message  over GSM network or store data within THINGSPEAK channel platform via GPRS connection ( module SIM800L is used for communication ). 
Connecting XTAL 8MHz as clock source is highly recommended in the design because it will ensure stability of serial communication between ATMEGA/ATTINY and SIM800L module. This makes especially the difference when environment temperature changes...

-----------------------------------------------------------------------------------------------------------------------------

1. Mobile phone as a target and text message option - files main.c + compileatmega (for ATMEGA328P) , main3.c + compileattiny (for ATTINY2313)

The video showig this mode is here : https://youtu.be/N060LW39fkQ


When a text message is sent to SIM card used within SIM800L module, the MCU ATTINY 2313 / ATMEGA 328P reads digital data from sensor DHT22 and sends response SMS to sender mobile.
The ATTINY 2313 / ATMEGA 328P and SIM800L are both put into sleep mode when there is no incoming messages so the power consumption is below 4mA.
ATTINY/ATMEGA interrupt pin INT0 is connected to SIM800L pin RING/RI as a wakeup signal. Pin RI/RING goes low when there is incoming text message on the SIM. ATTINY/ATMEGA wakes up and wakes up SIM800L module. That allows to conserve energy and ensures longest lifetime.


--------------------------------------------------------------------------------------------------------------------------------

2. THINGSPEAK platform as a target - files mainb + compileatmegab (for ATMEGA328P), main3b.c + compileattinyb (for ATTINY2313)

The video showing mode is working : https://www.youtube.com/watch?v=i4JgbwCktYQ

The file "main3b.c"/"compileattinyb" ( or "main3b.c"/"compileattinyb" with no-radio-off option)  and "mainb.c"/"compileatmegab"  ( or "mainc.c"/"compileatmegac" with no-radio-off-option )  are Thingspeak version. 
In this option MCU will inititate GPRS connection for 30 seconds every N minutes (here in the code N =  120 minutes) using SIM800L module, then it will contact Thingspeak server and send HTTP POST towards Thingspeak servers to store your measurements from DHT22 sensor.  Depending on selected option - between consecutive DHT22 measurements the SIM800L - has radio switched off or not - and it is put into SLEEP MODE to conserve power. Sometimes where measurement are more frequent ( less than 5 hours)  switching off radio is bad choice because consecutive registrations to GSM network use a lot of energy... Then simple SLEEP MODE on SIM800L is better...

How it works - details are here : https://www.teachmemicro.com/send-data-sim800-gprs-thingspeak/     and here   https://electronics-project-hub.com/send-data-to-thingspeak-arduino/
To use these source file you have to create Thingspeak account and get API key first, then an update of source file is needed.
API Key must be inserted into "main3b.c"/"mainb.c" source file as well as APN settings for GPRS access from your SIM card  ( it is marked in remarks in the code).

--------------------------------------------------------------------------------------------------------------------

3. Other considerations : 

SIM800L should be first configured to work on serial port with speed 9600bps. 
The device can be powerd from 3xAA bateries or combination of LiIon 3.7V rechargable battery and 4V Solar Cell so it can be put outdoor. Currently I am using 4xAA NiMH battery pack ( 1.2V x 4 cells ) with voltage drop down diode 1N4007 in serial (around ~0,6V voltage drop so result is 4.2V for powering SIM800L and MCU) and this gives very good results in powering this IoT device. 
SIM800L requires good power source since it can draw up to 2A of current during short peaks. Ensure that you have good cabling and good power source.
Please use XTAL 8MHz + 2 x 22pF capacitors to ensure stability of serial connection between MCU ans SIM800L. To reprogram chip to use quartz change L-FUSE value to "7F" in compileattinyX / compileatmegaX batch script in AVRDUDE command section. 
Internal RC clock generator may be very unstable and prevent device from working.




---------------------------------

Part List :

- ATTINY 2313  or ATMEGA 328P chip
- XTAL 8MHz - recommended for communication stability 
- SIM800L dev board
- DHT22 sensor ( for outdoor reading ) or DHT11 sensor ( for indoor temperatures )
- 1000uF/10V capacitor ( or higher ) 
- 100nF capacitor
- 2 x 22pF caapacitors when using XTAL
- 3 x AA batteries or    
- Solar Cell 4 V - 5V   and diode 1N4007 and LiIon 3.7V battery ( same as used in mobile phones )

---------------------------------

connections to be made  : 


a) ATMEGA328P option

 ATMEGA328P : INT0 pin (#4) of ATMEGA328P must be connected to RI/RING ping on SIM800L module
 
 SIM800L RXD to ATMEGA328 TXD PIN #3,
 
 SIM800L TXD to ATMEGA328 RXD PIN #2
 
 DHT11/DHT22 sensor pin DATA is connected to ATMEGA328 PB0 PIN #14
 
 ATMEGA328 VCC (PIN #7) to SIM800L VCC , DHT22 pin VCC and +4V power socket
 
 ATMEGA328 GND (PIN #8 and PIN #22) to SIM800L GND , DHT22 pin GND and 0V of power socket
 
 3xAA battery pack must be connected : "+" to VCC line, "-" to GND line
 
 ( optional ) XTAL 8MHz -  put between pins 9 & 10 of ATMEGA 328P
 
 (optional for XTAL ) 22pF capacitors put between ATMEGA 328 P : pins 9 and GND , between pin 10 and GND 
 
----------


b) ATTINY2313 option

 ATTINY2313 : INT0 pin (#6) must be connected to RI/RING ping on SIM800L module
 
 SIM800L RXD to ATTINY2313 TXD PIN #3,
 
 SIM800L TXD to ATTINY2313 RXD PIN #2
 
 DHT11/DHT22 sensor pin DATA is connected to ATTINY2313 PB0 PIN #12
 
 ATTINY2313 VCC (PIN #20) to SIM800L VCC , DHT22 pin VCC and +4V power socket
 
 ATTINY2313 GND (PIN #10) to SIM800L GND , DHT22 pin GND and 0V of power socket
 
 3xAA battery pack must be connected : "+" to VCC line, "-" to GND line
 
( optional ) XTAL 8MHz -  put between pins 4 & 5 of ATTINY2313
 
 (optional for XTAL ) 22pF capacitors put between ATTINY2313 : pins 4 and GND , between pin 5 and GND 
 
---------------------------------

How to compile and upload code to the chip ATTINY 2313 or ATMEGA 328P ??
See this tutorial https://www.youtube.com/watch?v=7klgyNzZ2TI

files : 
- compileattiny and main3.c  are used for chip ATTINY2313
- compileatmega and main.c   are used for chip ATMEGA328P
- compileattinyb and main3b.c  are used for chip ATTINY2313
- compileatmegab and mainb.c   are used for chip ATMEGA328P
- compileattinyc and main3c.c  are used for chip ATTINY2313
- compileatmegac and mainc.c   are used for chip ATMEGA328P


What do you need :
1. USBASP programmer
2. Six  color wires to connect the chip with USBASP Kanda socket
3. Solderless breadboard on which you put ATTINY / ATMEGA chip and  connect cabling from USBASP 
4. PC with Ubuntu Linux ( version does not matter)  with following packages installed :  "gcc-avr", "binutils-avr" (or sometimes just "binutils"), "avr-libc", "avrdude"  - the AVR-GCC environment and AVRDUDE flash programmer

