# XY Wifi Clock Replacement Firmware
This is a replacement firmware for the XY-Clock with Wifi (and similar variants) found at https://www.aliexpress.com/item/1005002686302823.html that eliminates the need to use the Sinilink app to control it. It is still early days in the development of this, so contributions and improvements are welcome. The pbjective has been to get a working end to end replacement for the current firmware that has the following features:
* No need to install mobile app. Functionality controlled from ESP8285 on the clock.
* Easy setup of connection to Wifi network using temporary access point.
* Synchronisation with a Network Time Protocol server (NTP) to get the correct time.
* After a power failure it should be able to restore the correct time without any user intervention (saved Wifi credentials with specified timezone).

Note that the ability to change the timezone is on the to do list. For the time being you will need to look at the available list of timezones in UpdateTimeTask.h and change the line updateTimeTask->setTimeZone("BST") to match your target timezone. Since the list is somewhat limited at the moment there is a decent chance you may need to add your timezone.

## To Do
Since this is early days in the development of the firmware there is still quite a bit to do, including the following in order of priority:
 - [ ] Add a configuration page that allows the user to change the timezone.
 - [ ] Add the ability to add/edit alarms
 - [ ] Add functionality to the K_UP,  K_DOWN and K_SET buttons
 - [ ] Use the LEDs to indicate state
 - [ ] Configure different alarm sounds
 - [ ] Create a 3D printable case for the clock
 - [ ] Leverage the other functionality available on the board, such as the DM1307Z+ real-time clock. 
 - [ ] Support playing music with versions that include an [MP3 Decoding board and Bluetooth 5.0](https://www.aliexpress.com/item/1005002933216217.html)
 
## Current Issues
* The TM1650 Segmented LED display has a slight flicker. Attempts at using different TM16xx libraries have been made but there appears to be some sort of conflict with the current libraries preventing them from working properly.
* The TM1650 ColonOn function appears to also highlight the two points at the bottom of the display. This will have something to do with the bitmap settings in the library.

## Using the new Firmware
* When using it for the first time it will create a Wifi access point called XY-Clock. Connect to this and select the wifi network and enter the password. It will then connect to the Wifi Network and sync the time with the pool.ntp.org Network Time Protocol server.

<p align="center">
    <img src="https://camo.githubusercontent.com/25be9c499e4b7b5e01a8bce1c58263084c40f3b2c4647f5c9ef62d6d5a3c9792/687474703a2f2f692e696d6775722e636f6d2f595076573965716c2e706e67" height="300px" />
</p>

* It uses the configured timezone to then display the correct time for your region.

You will need to install support for the ESP8266 chip in the Arduino IDE as well as the following libraries in order to get the firmware to compile:
* WiFiManager by Tzapu: https://github.com/tzapu/WiFiManager
* ArduinoJson: https://github.com/bblanchon/ArduinoJson
* ESP8266 Scheduler: https://github.com/nrwiersma/ESP8266Scheduler
* NTPClient: https://github.com/arduino-libraries/NTPClient
* Time: https://github.com/PaulStoffregen/Time
* Timezone by Jack Christensen: https://github.com/JChristensen/Timezone

## Reprogramming the Wifi Clock
You can use an FTDI programmer and empty pins at the top left of the board to reprogram the ESP8285. You can use breadboard jumper wires to connect it up to the FTDI programmer. Make sure you put the FTDI programmer in 3.3V mode (you will destroy the chip if the FTDI is in 5V mode). The layout of the pins on the PCB are as follows:

![image](./Documentation/Images/XY-Clock-Labelled.jpg)

You need to connect the FTDI to the board in the following way:

| Pin  | Function | FTDI |
| ---- | -------- |----- |
| 1    | GND      | GND  |
| 2    | TX       | RX   |
| 3    | RX       | TX   |
| 4    | GPIO0    | GND when flashing firmware |
| 5    | CHIP_EN  |      |
| 6    | VCC      | VCC  |

When you want to flash the ESP8285 chip you need to connect GPIO0 to GND before you connect the FTDI programmer to the computer (before you plug it into the USB port). You can then flash the new firmware on to the board. Once the new firmware has been uploaded disconnect GPIO0 from GND and reset the device to run it. You can just unplug the FTDI from the USB port and plug it back in which will resupply power to the board and allow you to use the Serial Monitor to debug it. You may want to uncomment the delay code in the Arduino sketch startup code to give your computer time to connect to the serial output.

![image](./Documentation/Images/Flashing-wifi-clock.jpg)

## Thanks
This work is built on top investigations and information that others have gathered. In particular I want to thank:
* Stefan Oskamp (@stefan-oskamp) for working out the GPIO connections on the board
* @sfromis, @MacSass and others who contributed to the discussion on https://github.com/arendst/Tasmota/discussions/15788