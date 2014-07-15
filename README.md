arduino
=======

Arduino projects

Here are some of my small arduino projects

weather_station.ino : a very simple weither station code to display temperature (in °C) and pressure (in mbar) using Leonardo or Yun Arduino platform + BMP180 Sensor + Arduino TFT Screen

http://arduino.cc/en/Main/ArduinoBoardYun

http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

http://arduino.cc/en/Main/GTFT

Current binary is around 20k. I'm trying to reduce its size to allow new features integration.


Wiring

Arduino Yun <-> BMP180
- 5V <-> VIN
- GND <-> GND
- DIGITAL3 <-> SCL
- DIGITAL2 <-> SDA

Arduino Yun <-> TFT
- +5V <-> +5V
- ICSP MISO <-> MISO
- ICSP SCK <-> SCK
- ICSP MOSI <-> MOSI
- DIGITAL7 <-> LCD CS
- DIGITAL8 <-> SD CS
- DIGITAL0 <-> D/C
- DIGITAL1 <-> Reset
- +5V <-> BL
- GND <-> Ground
