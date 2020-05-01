# smartmetering
Smart metering of temperature and humidity using GSM network and SMS text messages

This is very simple example how to build IoT device that sends remote reading of temperature and humidity ( from DHT22 / DHT11 sensor) with text message  over GSM network ( module SIM800L is used for communication ). 
When a text message is sent to SIM card used within SIM800L module, the MCU ATTINY 2313 reads digital data from sensor DHT22 and sends response SMS to sender mobile.

The ATTINY 2313 and SIM800L are both put into sleep mode when there is no incoming messages so the power consumption is below 4mA.

The device can be powerd from 3xAA bateries or combination of LiIon 3.7V rechargable battery and 4V Solar Cell so it can be put outdoor.

