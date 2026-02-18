This repo container the firmware needed to run the usbc-midi adapter



# To build the code

* Download Adruino IDE 
* Install https://github.com/earlephilhower/arduino-pico @ 5.5.0 to support RP2040 Boards
* Make sure you select 120MHz and the usb stack to Adafruit TinyUSB

To compile in terminal
```
arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v ./rp2040
```

To upload in terminal

```
arduino-cli compile --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb -v ./rp2040 --upload --port /dev/ttyACM0
```

To Monitor
```
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```
## Required Arduino Libraries

```
Used library             Version Path
Pico PIO USB             https://github.com/Hanz-Tech/Pico-PIO-USB
MIDI Library             5.0.2   
Adafruit TinyUSB Library 3.7.4  copy to Arduino15/packages/rp2040/hardware/rp2040/5.5.0/libraries/Adafruit_TinyUSB_Arduino if needed
SPI                      1.0    
ArduinoJson              7.4.1   
EEPROM                   1.0     
```