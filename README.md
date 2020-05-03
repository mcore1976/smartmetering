# smartmetering
Smart metering of temperature and humidity using GSM/GPRS network and SMS text messages or THINGSPEAK platform.

This is very simple example how to build IoT device that sends remote reading of temperature and humidity ( from DHT22 / DHT11 sensor) with text message  over GSM network or store data within THINGSPEAK platform with GPRS ( module SIM800L is used for communication ). 

1. Mobile phone as a target and text message option - files main.c + compileatmega (for ATMEGA328P) , main3.c + compileattiny (for ATTINY2313)

When a text message is sent to SIM card used within SIM800L module, the MCU ATTINY 2313 / ATMEGA 328P reads digital data from sensor DHT22 and sends response SMS to sender mobile.
The ATTINY 2313 / ATMEGA 328P and SIM800L are both put into sleep mode when there is no incoming messages so the power consumption is below 4mA.
ATTINY/ATMEGA interrupt pin INT0 is connected to SIM800L pin RING/RI as a wakeup signal. Pin RI/RING goes low when there is incoming text message on the SIM. ATTINY/ATMEGA wakes up and wakes up SIM800L module. That allows to conserve energy and ensures longest lifetime.

2. THINGSPEAK platform as a target - files mainb + compileatmegab (for ATMEGA328P), main3b.c + compileattinyb (for ATTINY2313)

The file "main3b.c"/"compileattinyb" and "mainb.c"/"compileattiny" are Thingspeak version. 
In this option MCU will inititate GPRS connection using SIM800L module every N minutes, then will contact Thingspeak server and make HTTP PUT to store your measurements from DHT22 sensor. How it works - details are here : https://www.teachmemicro.com/send-data-sim800-gprs-thingspeak/     and here   https://electronics-project-hub.com/send-data-to-thingspeak-arduino/
To use these source file you have to create Thingspeak account and get API key first, then an update of source file is needed.
API Key must be inserted into "main3b.c"/"mainb.c" source file as well as APN settings for GPRS access from your SIM card  ( it is marked in remarks in the code).


3. Other considerations : 

SIM800L should be first configured to work on serial port with speed 9600bps. 
The device can be powerd from 3xAA bateries or combination of LiIon 3.7V rechargable battery and 4V Solar Cell so it can be put outdoor.



You can see how this device works here : https://youtu.be/N060LW39fkQ

---------------------------------

Part List :

- ATTINY 2313  or ATMEGA 328P chip
- SIM800L dev board
- DHT22 sensor ( for outdoor reading ) or DHT11 sensor ( for indoor temperatures )
- 1000uF/10V capacitor ( or higher ) 
- 100nF capacitor
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



b) ATTINY2313 option

 ATTINY2313 : INT0 pin (#6) must be connected to RI/RING ping on SIM800L module
 
 SIM800L RXD to ATTINY2313 TXD PIN #3,
 
 SIM800L TXD to ATTINY2313 RXD PIN #2
 
 DHT11/DHT22 sensor pin DATA is connected to ATTINY2313 PB0 PIN #12
 
 ATTINY2313 VCC (PIN #20) to SIM800L VCC , DHT22 pin VCC and +4V power socket
 
 ATTINY2313 GND (PIN #10) to SIM800L GND , DHT22 pin GND and 0V of power socket
 
 3xAA battery pack must be connected : "+" to VCC line, "-" to GND line
 

---------------------------------

How to compile and upload code to the chip ATTINY 2313 ??
See this tutorial https://www.youtube.com/watch?v=7klgyNzZ2TI

files : 
- compileattiny and main3.c  are used for chip ATTINY2313
- compileatmega and main.c   are used for chip ATMEGA328P
- compileattinyb and main3b.c  are used for chip ATTINY2313
- compileatmegab and mainb.c   are used for chip ATMEGA328P


What do you need :
1. USBASP programmer
2. Six  color wires to connect the chip with USBASP Kanda socket
3. Solderless breadboard on which you put ATTINY / ATMEGA chip and  connect cabling from USBASP 
4. PC with Ubuntu Linux ( version does not matter)  with following packages installed :  "gcc-avr", "binutils-avr" (or sometimes just "binutils"), "avr-libc", "avrdude"  - the AVR-GCC environment and AVRDUDE flash programmer

