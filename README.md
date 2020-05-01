# smartmetering
Smart metering of temperature and humidity using GSM network and SMS text messages

This is very simple example how to build IoT device that sends remote reading of temperature and humidity ( from DHT22 / DHT11 sensor) with text message  over GSM network ( module SIM800L is used for communication ). 
When a text message is sent to SIM card used within SIM800L module, the MCU ATTINY 2313 reads digital data from sensor DHT22 and sends response SMS to sender mobile.

The ATTINY 2313 and SIM800L are both put into sleep mode when there is no incoming messages so the power consumption is below 4mA.
ATTINY interrupt pin INT0 is connected to SIM800L pin RING/RI as a wakeup signal. Pin RI/RING goes low when there is incoming text message on the SIM. ATTINY wakes up and wakes up SIM800L module. That allows to conserve energy and ensures longest lifetime.
SIM800L should be first configured to work on serial port with speed 9600bps. 

The device can be powerd from 3xAA bateries or combination of LiIon 3.7V rechargable battery and 4V Solar Cell so it can be put outdoor.

Part List :

- ATTINY 2313 
- SIM800L dev board
- DHT22 sensor ( for outdoor reading ) or DHT11 sensor ( for indoor temperatures )
- 1000uF/10V capacitor ( or higher ) 
- 100nF capacitor
- 3 x AA batteries or    
- Solar Cell 4 V - 5V   and diode 1N4007 and LiIon 3.7V battery ( same as used in mobile phones )

How to compile and upload code to the chip ATTINY 2313 ??
See this tutorial https://www.youtube.com/watch?v=7klgyNzZ2TI

What do you need :
1. USBASP programmer
2. Six  color wires to connect the chip with USBASP Kanda socket
3. Solderless breadboard on which you put ATTINY / ATMEGA chip and  connect cabling from USBASP 
4. PC with Ubuntu Linux ( version does not matter)  with following packages installed :  "gcc-avr", "binutils-avr" (or sometimes just "binutils"), "avr-libc", "avrdude"  - the AVR-GCC environment and AVRDUDE flash programmer
