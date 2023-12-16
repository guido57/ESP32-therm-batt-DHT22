# ESP32 Solar Powered WiFi Thermometer and Hygrometer 
A home wifi thermostat with a Raspberry PI-3 with TFT screen and an ESP8266  measuring the room temperature and sending it to Raspberry via wifi.  

### Overview
This wifi thermometer based on a DHT22 sensor monitors temperature and humidity in my garden

[![](https://github.com/guido57/MyThermostat/blob/master/off.PNG)](https://github.com/guido5

### Logic Diagram 
[![N|Solid](https://github.com/guido57/MyThermostat/blob/master/Logic%20Diagram%20And%20Schematic%20v1.PNG)](https://github.com/guido57/MyThermostat/blob/master/Logic%20Diagram%20And%20Schematic%20v1.PNG)

### 220V relay connection to Raspberry PI Schematic
See these lines of code inside MultiPage.py

```
 # set pin 40 as output to command the 220V relay
 RELAY_PIN = 40
 ...
 import RPi.GPIO as GPIO
 GPIO.setmode(GPIO.BOARD)
 GPIO.setup(RELAY_PIN,GPIO.OUT)
 GPIO.output(RELAY_PIN,GPIO.HIGH)
```            
