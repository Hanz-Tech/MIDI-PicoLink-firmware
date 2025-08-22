This repo container the firmware needed to run the usbc-midi adapter



# To build the code

* Download Adruino IDE 
* Install https://github.com/earlephilhower/arduino-pico @ 4.7.1 to support RP2040 Boards
* Make sure you select 120MHz and the usb stack to Adafruit TinyUSB

To compile in terminal
```
arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v
```

## Required Arduino Libraries

Used library             Version Path
Pico PIO USB             0.7.2   
MIDI Library             5.0.2   
Adafruit TinyUSB Library 3.7.1   Arduino15/packages/rp2040/hardware/rp2040/4.7.1/libraries/Adafruit_TinyUSB_Arduino
SPI                      1.0     Arduino15/packages/rp2040/hardware/rp2040/4.7.1/libraries/SPI
ArduinoJson              7.4.1   
EEPROM                   1.0     