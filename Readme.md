# XY-Wifi Clock Replacement Firmware

This is a replacement firmware for the XY-Clock with Wifi (and similar variants) found at https://www.aliexpress.com/item/1005002686302823.html and https://www.aliexpress.com/item/1005002941223785.html that eliminates the need to use the Sinilink app to control it. It is still early days in the development of this proof of concept/beta project, so contributions and improvements are welcome. The objective has been to get a working end to end replacement for the current firmware that has the following features:
* No need to install mobile app. Functionality controlled from ESP8285 on the clock.
* Easy setup of connection to Wifi network using temporary access point.
* Synchronisation with a Network Time Protocol server (NTP) to get the correct time.
* A configuration web page that allows you to:
  * Change the timezone and device name and 12 or 24-hour mode
  * Change brightness based on the time of day
  * Setup up to 6 alarms (to be activated on selected days)
* After a power failure it should be able to restore the correct time without any user intervention (saved Wifi credentials with specified timezone).

## To Do

Since this is early days in the development of the firmware there is still quite a bit to do, including the following in order of priority:
 - [ ] Add/improve functionality to the K_UP, K_DOWN and K_SET buttons
 - [ ] Use the LEDs to indicate state
 - [ ] Configure different alarm sounds
 - [ ] Create a 3D printable case for the clock
 - [ ] Leverage the other functionality available on the board, such as the DM1307Z+ real-time clock. 
 - [ ] Support playing music with versions that include an [MP3 Decoding board and Bluetooth 5.0](https://www.aliexpress.com/item/1005002933216217.html)
 

## Flashing the Wifi Clock

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

![image](./Documentation/Images/Flashing-wifi-clock.jpg)

GPIO0 must be connected to GND to put the ESP8285 into flashing mode.  You can do this before connecting the FTDI programmer to the computer or before connecting the VCC pins.  Either way, it is optional to leave it connected during the flashing process.

After the first flash and upload of Sketch Data, you can use ArduinoOTA to update it.  It may be important to always use the same board configuration as you used during the initial flash so as to keep SPIFFs intact. I recommend setting to Board: Generic ESP8266 with 2MB Flash / FS:256KB.

Once the new firmware has been uploaded, disconnect GPIO0 from GND and reset the device to run it. You can just unplug the FTDI from the USB port and plug it back in which will resupply power to the board and allow you to use the Serial Monitor to debug it.

### SPIFFs

For now, this firmware requires that you add SPIFFs sketch data as a second step in the flashing process.  Use the same method as above to put the ESP into flashing mode.

You will need the ESP8266 File System Uploader plugin for arduino IDE to upload the config web page to the device.  Get it here: https://github.com/esp8266/arduino-esp8266fs-plugin.  This plugin is only compatible with Legacy Arduino IDE 1.8 (not the newer 2).

Once you have installed the plugin and restarted the IDE, open up the project and use the Tools -> ESP866 Sketch Data Upoad option to upload the data folder (including the Config.html) file. 

When this project is migrated to platform.io in the future we will use this approach: https://docs.platformio.org/en/latest/platforms/espressif8266.html#uploading-files-to-filesystem so this step will just be part of the standard build and deployment process.


### Arduino Libraries Used

You will need to install support for the ESP8266 chip in the Arduino IDE as well as the following libraries in order to get the firmware to compile:
* WiFiManager by Tzapu: https://github.com/tzapu/WiFiManager
* ArduinoJson: https://github.com/bblanchon/ArduinoJson
* Time: https://github.com/PaulStoffregen/Time
* Adafruit_GFX by Adafruit: https://github.com/adafruit/Adafruit-GFX-Library
* TM16xx LEDs and Buttons by Maxint: https://github.com/maxint-rd/TM16xx
* ArduinoOTA by JAndrassy: https://github.com/jandrassy/ArduinoOTA
* Button2 by Lennart Hennigs: https://github.com/LennartHennigs/Button2
* NTPClient by Fabrice Weinberg: https://github.com/arduino-libraries/NTPClient


## How to Use This Firmware

* When using it for the first time it will create a Wifi access point called XYClock. Connect to this and select the wifi network and enter the password. It will then connect to the Wifi Network and sync the time with the pool.ntp.org Network Time Protocol server.

<img src="https://camo.githubusercontent.com/25be9c499e4b7b5e01a8bce1c58263084c40f3b2c4647f5c9ef62d6d5a3c9792/687474703a2f2f692e696d6775722e636f6d2f595076573965716c2e706e67" height="300px" />

* It uses the configured timezone to then display the correct time for your region. The Wifi clock will attempt to setup an mDNS entry for http://xy-clock.local or the device name you specify. While mDNS is apparently natively supported in Windows 10 build 1703 and later, if it doesn't work for you on Windows you may need to install the Apple Bonjour service ([download link](https://support.apple.com/kb/dl999)). That being said, you may have better success using the IP address of the clock.

* The config page looks like the screenshot below. The focus at the moment is functionality rather than looks, but at some point this needs to have better styling applied to it.

![image](./Documentation/Images/XY-Clock-Config.png)

Still in progress but so far, the buttons can be used as such:
| Button  | Short-press Function | Long-press Function (hold for 3 seconds) |
| ------- | -------------------- |----------------------------------------- |
| Up      | Increase Brightness  | - |
| Down    | Decrease Brightness | - |
| Set     | Show the Clock's IP Address | Turn on/off the hotspot |

###  Function Notes

Changing the brightness using the buttons is temporary and will reset whenever the brightness on the web UI is altered or the brightness is changed by the Day Start or Night Start times.

The hotspot will be created using the device name from the configuration page.  The configuration page can then be accessed through http://192.168.4.1 (which is an ESP device's default).  The hotspot will be automatically turned off after 5 minutes.

There some quirks and known issues with ESP devices, especially when accessing the hotspot on certain Android phones.   First, it may not correctly load the clock's configuration if the mobile device still has data mode on.  So, try turning off mobile data when loading the webpage.  Second, it appears that Chrome mobile may work better - if there is an error, you can simply pull down to refresh the page (unlike some other mobile browsers).  Finally, it's important to know that changing the time zone or device name WILL reset the device, turning off the hotspot, so you may need to repeat turning on and connecting to the hotspot after changing timezones.


## Thanks
This work is built on top investigations and information that others have gathered. In particular I want to thank:
* Stefan Oskamp (@stefan-oskamp) for working out the GPIO connections on the board
* @sfromis, @MacSass and others who contributed to the discussion on https://github.com/arendst/Tasmota/discussions/15788
