// Distributed under AGPL v3 license. See license.txt

// downloaded libraries
#include <WiFiManager.h>                        // https://github.com/tzapu/WiFiManager             // if using Arduino IDE, click here to search: http://librarymanager#WiFiManager_ESPx_tzapu
#include <ArduinoJson.h>                        // https://github.com/bblanchon/ArduinoJson         // if using Arduino IDE, click here to search: http://librarymanager#ArduinoJson
#include <TM1650.h>                             // https://github.com/maxint-rd/TM16xx              // if using Arduino IDE, click here to search: http://librarymanager#TM16xx_LEDs_and_Buttons
#include <TM16xxDisplay.h>                      // ...
#include <Button2.h>                            // https://github.com/LennartHennigs/Button2        // if using Arduino IDE, click here to search: http://librarymanager#Button2

// libraries from ESP core
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// local includes
#include "DS1307.h"
#include "pitches.h"
#include "XY-Wifi-Clock.h"


// I2C pins, used for TM1650 and DS1307 chips
const int SCL_PIN = 12;
const int SDA_PIN = 13;

#define NUM_DIGITS 4
TM1650 Disp4Seg(SDA_PIN, SCL_PIN);
TM16xxDisplay display(&Disp4Seg, NUM_DIGITS);

const int BUZZER_PIN = 5;

#define buttonUpPin     10
#define buttonDownPin   9
#define buttonSetPin    16
#define buttonKeyPin    14
Button2 buttonUp, buttonDown, buttonSet, buttonKey;

// blue LED ON indicates WiFi is connected
#define blue_LED_pin    0

// red LED is solid ON if NTP was successful, blinking if NTP didn't work
#define red_LED_pin     2

// external AM/PM LED is soldered between the PIN2 pad and GND
#define AM_PM_LED_pin   4
int last_AM_PM_brightness;
int AM_PM_values[9] =
{
    0,                                      // LED off
    1 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 1
    2 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 2
    3 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 3
    4 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 4
    5 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 5
    6 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 6
    7 * (AM_PM_BRIGHTNESS_FACTOR / 8),      // brightness 7
    8 * (AM_PM_BRIGHTNESS_FACTOR / 8)       // brightness 8
};

bool hasNetworkConnection = false;
unsigned long last_NTP_Update = 0 - TIME_UPDATE_INTERVAL;
WiFiManager wifiManager;

uint16_t  currentTime    = 0;       // (100 * hour) + minute
uint8_t   currentHour    = 0;
uint8_t   currentMinute  = 0;
uint8_t   currentSecond  = 0;
uint8_t   currentWeekDay = 0;       // 1-7, 1 = Monday
struct tm currentTimeInfo;

ESP8266WebServer server(80);

bool isTextOnDisplay = false;

#define HOTSPOT_CLOSED      0
#define HOTSPOT_OPEN        1
int hotspotState = HOTSPOT_CLOSED;
unsigned long hotspotOpenTime = 0;

#define MIN_BRIGHTNESS       1
#define MAX_BRIGHTNESS       8
uint8_t currentBrightness  = 0;     // current display brightness
uint8_t lastAutoBrightness = 0;     // last brightness set by auto day/night settings
uint8_t manualBrightness   = 4;     // brightness set by pushbutton

bool isSpiffsStarted = false;

bool buzzerOn = false;
unsigned long buzzerStartTime;

// prototype needed for class definition
void ensureSpiffsStarted();


class Alarm
{
    protected:
        bool    alarmEnable;
        uint8_t alarmHour;
        uint8_t alarmMinute;
        uint8_t lastHour   = -1;
        uint8_t lastMinute = -1;
        uint8_t daysOfWeek = 0;

    public:
        bool getEnable()
        {
            return alarmEnable;
        }

        void setEnable(bool value)
        {
            alarmEnable = value;
        }

        uint8_t getHour()
        {
            return alarmHour;
        }

        void setHour(uint8_t value)
        {
            alarmHour = value;
        }

        uint8_t getMinute()
        {
            return alarmMinute;
        }

        void setMinute(uint8_t value)
        {
            alarmMinute = value;
        }

        // Whether or not it is for the day of the week
        bool isForDayOfWeek(uint8_t day)
        {
            return (daysOfWeek & (1UL << day));
        }

        void clearDaysOfWeek()
        {
            daysOfWeek = 0;
        }

        void enableDayOfWeek(uint8_t day)
        {
            daysOfWeek |= (1u << day);
        }

        // Determines if the alarm has triggered
        bool hasAlarmTriggered(uint8_t hour, uint8_t minute, uint8_t day)
        {
            if (!alarmEnable  ||  (daysOfWeek == 0))
            {
                return false;
            }

            if (lastHour != hour || lastMinute != minute)
            {
                lastHour = hour;
                lastMinute = minute;

                bool isDayOfWeekMatch = isForDayOfWeek(day);
/*
                Serial.print("Alarm check ");
                Serial.print("Alarm ");
                Serial.print(alarmHour);
                Serial.print(":");
                Serial.print(alarmMinute);
                Serial.print(" Days: ");
                Serial.println(daysOfWeek);

                Serial.print("Current Time: ");
                Serial.print(hour);
                Serial.print(":");
                Serial.print(minute);
                Serial.print(" Week Day: ");
                Serial.print(day);
                Serial.print(" Is day match: ");
                Serial.println(isDayOfWeekMatch);
*/
                if ((hour == alarmHour)  &&  (minute == alarmMinute)  && (isDayOfWeekMatch))
                {
                    Serial.println("Alarm triggered");
                    return true;
                }
            }

            return false;
        }
};


class BrightnessAlarm
{
    private:
        uint8_t alarmHour;
        uint8_t alarmMinute;
        uint8_t alarmBrightness;

    public:
        BrightnessAlarm(uint8_t hour, uint8_t minute, uint8_t brightness)
        {
            alarmHour       = hour;
            alarmMinute     = minute;
            alarmBrightness = brightness;
        }

        void setBrightness(uint8_t value)
        {
            if (value > MAX_BRIGHTNESS) value = MAX_BRIGHTNESS;
            if (value < MIN_BRIGHTNESS) value = MIN_BRIGHTNESS;
            alarmBrightness = value;
        }

        uint8_t getBrightness()
        {
            return alarmBrightness;
        }

        uint8_t getHour()
        {
            return alarmHour;
        }

        void setHour(uint8_t value)
        {
            alarmHour = value;
        }

        uint8_t getMinute()
        {
            return alarmMinute;
        }

        void setMinute(uint8_t value)
        {
            alarmMinute = value;
        }
};


class Config
{
    private:
        String  deviceName;
        String  timezone;
        String  displayMode;
        int     displayModeNumber;
        String  dateFormat;
        int     dateFormatNumber;
        String  alarmSound;
        bool    autoBrightnessEnable;

    public:
        Alarm alarms[7]= { Alarm(), Alarm(), Alarm(), Alarm(), Alarm(), Alarm(), Alarm() };

        BrightnessAlarm* dayBrightnessAlarm   = new BrightnessAlarm( 6, 0, 3);
        BrightnessAlarm* nightBrightnessAlarm = new BrightnessAlarm(20, 0, 1);

        String getDeviceName()
        {
            if ((deviceName == NULL)  ||  (deviceName == ""))
            {
                deviceName = DEFAULT_APP_NAME;
            }

            return (deviceName);
        }

        void setDeviceName(String value)
        {
            deviceName = value;
        }

        String getTimezone()
        {
            return (timezone);
        }

        void setTimezone(String value)
        {
            timezone = value;
        }

        String getDisplayMode()
        {
            return (displayMode);
        }

        void setDisplayMode(String value)
        {
            displayMode = value;
        }

        int getDisplayModeNumber()
        {
            return (displayModeNumber);
        }

        void setDisplayModeNumber(int value)
        {
            displayModeNumber = value;
        }

        int convertDisplayModeToNumber(String value)
        {
            ensureSpiffsStarted();

            File displayModesFile = SPIFFS.open("/displayModes.json", "r");

            DynamicJsonDocument doc(512);
            deserializeJson(doc, displayModesFile);
            displayModesFile.close();

            int temp = doc[value];

            doc.garbageCollect();

            return (temp);
        }

        String getDateFormat()
        {
            return (dateFormat);
        }

        void setDateFormat(String value)
        {
            dateFormat = value;
        }

        int getDateFormatNumber()
        {
            return (dateFormatNumber);
        }

        void setDateFormatNumber(int value)
        {
            dateFormatNumber = value;
        }

        int convertDateFormatToNumber(String value)
        {
            ensureSpiffsStarted();

            File dateFormatsFile = SPIFFS.open("/dateFormats.json", "r");

            DynamicJsonDocument doc(512);
            deserializeJson(doc, dateFormatsFile);
            dateFormatsFile.close();

            int temp = doc[value];

            doc.garbageCollect();

            return (temp);
        }

        String getAlarmSound()
        {
            return (alarmSound);
        }

        void setAlarmSound(String value)
        {
            alarmSound = value;
        }

        int convertAlarmSoundToNumber(String value)
        {
            ensureSpiffsStarted();

            File alarmSoundsFile = SPIFFS.open("/alarmSounds.json", "r");

            DynamicJsonDocument doc(512);
            deserializeJson(doc, alarmSoundsFile);
            alarmSoundsFile.close();

            int temp = doc[value];

            doc.garbageCollect();

            return (temp);
        }

        bool getAutoBrightnessEnable()
        {
            return autoBrightnessEnable;
        }

        void setAutoBrightnessEnable(bool value)
        {
            autoBrightnessEnable = value;
        }
};
Config config;


class Timezones
{
    public:
        // Get timezone posix string if it is a valid timezone
        static String getTimezonePosixText(String olsonTimezone)
        {
            ensureSpiffsStarted();

            File timeZonesFile = SPIFFS.open("/timezones.json", "r");

            DynamicJsonDocument doc(16384);
            deserializeJson(doc, timeZonesFile);
            timeZonesFile.close();

            JsonObject documentRoot = doc.as<JsonObject>();
            String posixText = documentRoot[olsonTimezone];

            Serial.print("Checking timezone: ");
            Serial.print(olsonTimezone);
            Serial.print(". Posix text found: ");
            Serial.println(posixText);

            doc.garbageCollect();

            if (posixText == NULL)
            {
                posixText = "GMT0";
            }

            return posixText;
        }

        // Is valid timezone name
        static bool isValidTimezone(String olsonTimezone)
        {
            ensureSpiffsStarted();

            File timeZonesFile = SPIFFS.open("/timezones.json", "r");

            DynamicJsonDocument doc(16384);
            deserializeJson(doc, timeZonesFile);
            timeZonesFile.close();

            JsonObject documentRoot = doc.as<JsonObject>();
            String posixText = documentRoot[olsonTimezone];

            doc.garbageCollect();

            if (posixText == NULL)
            {
                return false;
            }

            return true;
        }
};


// setup the clock
void setup()
{
    Serial.begin(115200);

    Serial.print("Starting ");
    Serial.println(DEFAULT_APP_NAME);
    Serial.println("Built on " __DATE__ " at " __TIME__);

    // setup rear panel LEDs as outputs and start with LEDs off
    pinMode(blue_LED_pin, OUTPUT);
    digitalWrite(blue_LED_pin, HIGH);
    pinMode(red_LED_pin, OUTPUT);
    digitalWrite(red_LED_pin, HIGH);

    // setup buzzer pin as an output
    pinMode(BUZZER_PIN, OUTPUT);

    // setup AM/PM LED pin as an output (default to off)
    pinMode(AM_PM_LED_pin, OUTPUT);
    last_AM_PM_brightness = 0;
    analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);

    // do a display self-test
    displayTest();

    // check if WiFi settings should be reset,
    // this is done by holding down the set
    // button for 5 seconds during power-up
    if (digitalRead(buttonSetPin) == LOW)
    {
        delay(1000);
        if (digitalRead(buttonSetPin) == LOW)
        {
            display.println("5");
            delay(1000);
            if (digitalRead(buttonSetPin) == LOW)
            {
                display.println("4");
                delay(1000);
                if (digitalRead(buttonSetPin) == LOW)
                {
                    display.println("3");
                    delay(1000);
                    if (digitalRead(buttonSetPin) == LOW)
                    {
                        display.println("2");
                        delay(1000);
                        if (digitalRead(buttonSetPin) == LOW)
                        {
                            display.println("1");
                            wifiManager.resetSettings();
                            delay(500);
                        }
                    }
                }
            }
        }
    }

    // display "conn" to mean waiting for a network connection
    display.println("conn");

    // try to connect
    Serial.println("Starting Wifi Manager");
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);         // workaround for bug where clock won't reconnect to an Asus router after DHCP lease expires (ESP core problem?)
    wifiManager.setDebugOutput(true);
    wifiManager.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    if (wifiManager.getWiFiIsSaved())
    {
        wifiManager.setEnableConfigPortal(false);
    }
    hasNetworkConnection = wifiManager.autoConnect(DEFAULT_APP_NAME);
    if (hasNetworkConnection)
    {
        Serial.println("Connected to WiFi Network");
        digitalWrite(blue_LED_pin, LOW);
    }
    else
    {
        Serial.println("Failed to connect to WiFi Network");
        displayScrollingText("conn FAIL");
    }

    // Button2 setup
    buttonUp.begin(buttonUpPin);
    buttonUp.setClickHandler(click);
    buttonUp.setLongClickHandler(longClick);
    buttonDown.begin(buttonDownPin);
    buttonDown.setClickHandler(click);
    buttonDown.setLongClickHandler(longClick);
    buttonSet.begin(buttonSetPin);
    buttonSet.setClickHandler(click);
    buttonSet.setLongClickHandler(longClick);
    buttonKey.begin(buttonKeyPin);
    buttonKey.setClickHandler(click);
    buttonKey.setLongClickHandler(longClick);

    // DS1307 interface setup
    DS1307_Setup(SCL_PIN, SDA_PIN);

    // load settings from SPIFFS
    loadSettings();

    // setup timezone in our config data
    String timezone = config.getTimezone();
    if (!Timezones::isValidTimezone(timezone))
    {
        // if configured timezone is invalid, use GMT
        timezone = "Etc/GMT";
    }
    config.setTimezone(timezone);

    // get Posix timezone string, using data from https://github.com/nayarsystems/posix_tz_db
    String posixTimezoneText = Timezones::getTimezonePosixText(timezone);
    Serial.print("Timezone posix text: ");
    Serial.println(posixTimezoneText);

    // start NTP Time sync service
    configTzTime(posixTimezoneText.c_str(), "pool.ntp.org", "time.nist.gov");

    // try NTP if there is a network connection
    int timeout = 0;
    if (hasNetworkConnection)
    {
        // display "ntp" to mean waiting for an NTP time update
        display.println("ntp");

        // wait until time is set from NTP or timeout expires
        //  on power-up, the ESP8285 starts the time at 1 Jan 1970 00:00:00 UTC
        //  so we check to see when the time is past New Year's day of 2000
        timeout = NTP_TIMEOUT;
        String text;
        Serial.print("Waiting for NTP time...");
        while ((time(nullptr) < 946706400)  &&  (timeout > 0))  // epoch time for 1 Jan 2000 00:00:00
        {
            if (timeout <= ((2 * NTP_TIMEOUT) / 3))
            {
                text = "-";
                text = rightJustifyNumber(text, timeout, " ");
                text.concat("-");
                display.println(text);
            }
            Serial.print(timeout);

            delay(1000);

            // note this backspace overwrite doesn't work on the Arduino IDE serial monitor, but works fine on Putty
            if (timeout >= 10)
            {
                Serial.print("\b\b  \b\b");
            }
            else
            {
                Serial.print("\b \b");
            }

            timeout--;
        }
        display.println("    ");
        Serial.println();
        if (timeout <= 0)
        {
            Serial.println("NTP update failed");
            displayScrollingText("ntp FAIL");
        }
    }

    if (timeout <= 0)
    {
        Serial.println("Trying to use DS1307 time...");
        DS1307_ReadTime();
    }
    else
    {
        // display update message
        timeUpdate();
    }

    // set NTP update callback
    settimeofday_cb(timeUpdate);

    updateDataFromConfig();

    // setup network stuff if connection was established
    if (hasNetworkConnection)
    {
        // Start the mDNS responder
        Serial.println("Setup MDNS ");
        if (MDNS.begin(config.getDeviceName()))
        {
            Serial.println("mDNS responder started");
        }
        else
        {
            Serial.println("Error setting up MDNS responder!");
        }

        // setup for OTA
        ArduinoOTA.setHostname((config.getDeviceName()).c_str()); // weird: this decides the mDNS name
        ArduinoOTA.onStart([]()
        {
            Serial.println("OTA Start");
            displaySomething("ota");
        });
        ArduinoOTA.onEnd([]()
        {
            Serial.println("\nOTA End");
            displaySomething("done");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
        {
            Serial.printf("Update Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error)
        {
            Serial.printf("OTA Error[%u]: ", error);
            displaySomething("fail");
            if (error == OTA_AUTH_ERROR) Serial.println("OTA Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("OTA Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("OTA End Failed");
        });
        ArduinoOTA.begin();

        // Display IP Address - uncomment if you want to see the IP at boot
        // displayIpAddress();

        startWebServer();

        MDNS.addService("http", "tcp", 80);
    }
}


// main loop
void loop()
{
    updateCurrentTime();
    yield();

    // only if not currently displaying text
    if (!isTextOnDisplay)
    {
        displayTime(currentHour, currentMinute, currentSecond);
        yield();
    }

    checkAlarms();
    yield();

    buttonUp.loop();
    buttonDown.loop();
    buttonSet.loop();
    buttonKey.loop();
    yield();

    checkHotspotTimeout();
    yield();

    checkAutoBrightness();
    yield();

    if (hasNetworkConnection)
    {
        server.handleClient();
        yield();

        MDNS.update();
        yield();

        ArduinoOTA.handle();
        yield();
    }

    int red_LED_state = LOW;
    if ((millis() - last_NTP_Update) > TIME_UPDATE_INTERVAL)
    {
        if ((currentSecond & 0x01) != 0)
        {
            red_LED_state = HIGH;
        }
    }
    digitalWrite(red_LED_pin, red_LED_state);

    // sound the buzzer if requested by an alarm
    if (buzzerOn)
    {
        static bool alreadyBuzzed = false;

        if ((currentSecond & 0x01) != 0)
        {
            if (!alreadyBuzzed)
            {
                beep(NOTE_C5, 100);
                yield();
                delay(200);
                beep(NOTE_C5, 100);
                yield();
                alreadyBuzzed = true;
            }
        }
        else
        {
            alreadyBuzzed = false;
        }

        // turn off buzzer if maximum time has been reached
        if ((millis() - buzzerStartTime) >= MAXIMUM_BUZZER_TIME)
        {
            buzzerOn = false;
        }
    }
    yield();
}


// replaces weak function from ESP core
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000()
{
    if (TIME_UPDATE_INTERVAL >= 15000)
    {
        return (TIME_UPDATE_INTERVAL);
    }
    else
    {
        return (SNTP_UPDATE_DELAY);
    }
}


// callback function to display NTP time update and update DS1307 clock chip
void timeUpdate()
{
    updateCurrentTime();

    char buffer[80];
    strftime(buffer, sizeof(buffer), "NTP time update to %A, %d %B %Y, %H:%M:%S", &currentTimeInfo);
    Serial.println(buffer);

    DS1307_WriteTime();

    last_NTP_Update = millis();
}


void ensureSpiffsStarted()
{
    if (isSpiffsStarted) return;

    if (!SPIFFS.begin())
    {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }

    Serial.println("SPIFFS started");
    isSpiffsStarted = true;
}


void updateDataFromConfig()
{
    int temp = config.convertDisplayModeToNumber(config.getDisplayMode());
    switch (temp)
    {
        case TIME_MODE_24_SOLID_COLON:
        case TIME_MODE_24_BLINKING_COLON:
        case TIME_MODE_12_SOLID_COLON:
        case TIME_MODE_12_SOLID_COLON_AM_LED:
        case TIME_MODE_12_SOLID_COLON_PM_LED:
        case TIME_MODE_12_BLINKING_COLON_ALWAYS:
        case TIME_MODE_12_BLINKING_COLON_AM:
        case TIME_MODE_12_BLINKING_COLON_PM:
            break;

        default:
            temp = TIME_MODE_12_SOLID_COLON;
            break;
    }
    config.setDisplayModeNumber(temp);

    temp = config.convertDateFormatToNumber(config.getDateFormat());
    switch (temp)
    {
        case DATE_FORMAT_MM_DD_YYYY:
        case DATE_FORMAT_DD_MM_YYYY:
            break;

        default:
            temp = DATE_FORMAT_MM_DD_YYYY;
            break;
    }
    config.setDateFormatNumber(temp);

    updateCurrentTime();
    if (!checkAutoBrightness())
    {
        lastAutoBrightness = manualBrightness;
        setDisplayBrightness(manualBrightness, false);
    }
}


// set/reset text on display variable, while keeping AM/PM LED off
void setTextOnDisplay(bool newState)
{
    isTextOnDisplay = newState;

    if (newState)
    {
        last_AM_PM_brightness = 0;
        analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
    }
}


// display scrolling text
void displayScrollingText(String text)
{
    setTextOnDisplay(true);

    int endPos = 0 - text.length() - 1;

    Serial.print("Displaying text ");
    Serial.print(text.length());
    Serial.print(" characters long: ");
    Serial.print(text);
    Serial.print(" End cursor pos: ");
    Serial.println(endPos);

    for (int cursorPosition = 0; cursorPosition > endPos; cursorPosition--)
    {
        display.setCursor(cursorPosition);
        display.println(text);
        delay(500);
        display.clear();
    }

    setTextOnDisplay(false);
}


void displayIpAddress()
{
    setTextOnDisplay(true);

    display.println("IP");
    delay(1500);

    IPAddress ipAddress = WiFi.localIP();

    for (int i = 0; i < 4; i++)
    {
        display.println(ipAddress[i]);
        delay(1500);
    }

    setTextOnDisplay(false);
}


void displaySomething(String text)
{
    setTextOnDisplay(true);

    display.println(text);

    setTextOnDisplay(false);
}


String rightJustifyNumber(String text, int number, String fillChar)
{
    if (number < 10)
    {
        text.concat(fillChar);
    }
    text.concat(String(number));
    return (text);
}


void displayAlarms()
{
    bool noAlarms    = true;
    int  alarmNumber = 0;

    setTextOnDisplay(true);

    for (auto& alarm : config.alarms)
    {
        alarmNumber++;

        if (alarm.getEnable())
        {
            noAlarms = false;

            last_AM_PM_brightness = 0;
            analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);

            display.print("AL ");
            display.println(alarmNumber);
            delay(1500);

            uint8_t hour   = alarm.getHour();
            uint8_t minute = alarm.getMinute();
            displayTime(hour, minute, 0);
            delay(1500);

            // blink the time if alarm is for later today or tomorrow
            bool blinkAlarm = false;
            if (alarm.isForDayOfWeek(currentWeekDay)  &&  (hour >  currentHour))
            {
                blinkAlarm = true;
            }
            if (alarm.isForDayOfWeek(currentWeekDay)  &&  (hour == currentHour)  &&  (minute > currentMinute))
            {
                blinkAlarm = true;
            }
            uint8_t testDay = (currentWeekDay + 1) % 7;
            if (testDay == 0)
            {
                testDay = 1;
            }
            if (alarm.isForDayOfWeek(testDay))
            {
                blinkAlarm= true;
            }
            if (blinkAlarm)
            {
                for (int i = 0; i < 2; i++)
                {
                    Disp4Seg.setSegments(0, 0);
                    Disp4Seg.setSegments(0, 1);
                    Disp4Seg.setSegments(0, 2);
                    Disp4Seg.setSegments(0, 3);
                    last_AM_PM_brightness = 0;
                    analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
                    delay(300);
                    displayTime(hour, minute, 0);
                    delay(300);
                }
            }
        }
    }

    if (noAlarms)
    {
        displayScrollingText("AL none");
    }

    setTextOnDisplay(false);
}


void displayDate()
{
    setTextOnDisplay(true);

    int year  = currentTimeInfo.tm_year + 1900;
    int month = currentTimeInfo.tm_mon  + 1;
    int day   = currentTimeInfo.tm_mday;

    // the four possible day-month digits
    uint8_t digit0;
    uint8_t digit1;
    uint8_t digit2;
    uint8_t digit3;

    String text = "";
    switch (config.getDateFormatNumber())
    {
        case DATE_FORMAT_MM_DD_YYYY:
            digit0 = month / 10;
            digit1 = month % 10;
            digit2 = day   / 10;
            digit3 = day   % 10;
            break;

        case DATE_FORMAT_DD_MM_YYYY:
            digit0 = day   / 10;
            digit1 = day   % 10;
            digit2 = month / 10;
            digit3 = month % 10;
            break;
    }
    if (digit0 == 0)
    {
        Disp4Seg.setSegments(0, 0);
    }
    else
    {
        Disp4Seg.setDisplayDigit(digit0, 0);
    }
    Disp4Seg.setDisplayDigit(digit1, 1, true);
    if (digit2 == 0)
    {
        Disp4Seg.setSegments(0, 2);
    }
    else
    {
        Disp4Seg.setDisplayDigit(digit2, 2);
    }
    Disp4Seg.setDisplayDigit(digit3, 3);
    delay(1500);

    display.println(year);
    delay(1500);

    setTextOnDisplay(false);
}


bool checkAutoBrightness()
{
    bool brightnessChanged = false;

    if (config.getAutoBrightnessEnable())
    {
        // get day/night times
        uint16_t dayBrightnessTime   = ((uint16_t)config.dayBrightnessAlarm->getHour()   * 100) + (uint16_t)config.dayBrightnessAlarm->getMinute();
        uint16_t nightBrightnessTime = ((uint16_t)config.nightBrightnessAlarm->getHour() * 100) + (uint16_t)config.nightBrightnessAlarm->getMinute();

        uint8_t autoBrightness;

        if (dayBrightnessTime < nightBrightnessTime)
        {
            if ((currentTime >= dayBrightnessTime)  &&  (currentTime <  nightBrightnessTime))
            {
                autoBrightness = config.dayBrightnessAlarm->getBrightness();
            }
            else
            {
                autoBrightness = config.nightBrightnessAlarm->getBrightness();
            }
        }
        else
        {
            // for those who stay up past midnight
            if ((currentTime <  dayBrightnessTime)  &&  (currentTime >= nightBrightnessTime))
            {
                autoBrightness = config.nightBrightnessAlarm->getBrightness();
            }
            else
            {
                autoBrightness = config.dayBrightnessAlarm->getBrightness();
            }
        }

        if (autoBrightness != lastAutoBrightness)
        {
            lastAutoBrightness = autoBrightness;
            manualBrightness   = autoBrightness;
            brightnessChanged  = true;
            Serial.println("Setting auto brightness");
            setDisplayBrightness(autoBrightness, false);
        }
    }

    return (brightnessChanged);
}


void setDisplayBrightness(uint8_t level, bool displayChanges)
{
    currentBrightness = level;

    Serial.print("Setting brightness to ");
    Serial.println(currentBrightness);

    Disp4Seg.setupDisplay(true, (currentBrightness - 1));

    if (displayChanges)
    {
        setTextOnDisplay(true);
        display.println(currentBrightness);
        delay(300);
        setTextOnDisplay(false);
    }
}


void increaseBrightness()
{
    if (manualBrightness < MAX_BRIGHTNESS)
    {
        Serial.println("Manual brightness increase");
        manualBrightness += 1;
        setDisplayBrightness(manualBrightness, true);
    }
}


void decreaseBrightness()
{
    if (manualBrightness > MIN_BRIGHTNESS)
    {
        Serial.println("Manual brightness decrease");
        manualBrightness -= 1;
        setDisplayBrightness(manualBrightness, true);
    }
}


// check the alarms
void checkAlarms()
{
    for (auto& alarm : config.alarms)
    {
        if (alarm.hasAlarmTriggered(currentHour, currentMinute, currentWeekDay))
        {
            soundAlarm(config.convertAlarmSoundToNumber(config.getAlarmSound()));
        }
    }
}


// update the current time
void updateCurrentTime()
{
    time_t rawtime;
    time(&rawtime);
    currentTimeInfo = *localtime(&rawtime);
    currentHour     = currentTimeInfo.tm_hour;
    currentMinute   = currentTimeInfo.tm_min;
    currentSecond   = currentTimeInfo.tm_sec;
    currentWeekDay  = currentTimeInfo.tm_wday;

    // adjust Sunday if needed
    if (currentWeekDay == 0)
    {
        currentWeekDay = 7;
    }

    currentTime = currentHour * 100 + currentMinute;
}


// these clicks can be pretty short! (so handling same as short-presses)
void click(Button2& btn)
{
    if (btn == buttonUp)
    {
        Serial.println("Up button clicked");
        increaseBrightness();
    }
    else if (btn == buttonDown)
    {
        Serial.println("Down button clicked");
        decreaseBrightness();
    }
    else if (btn == buttonSet)
    {
        Serial.println("Set button clicked");
        displayIpAddress();
    }
    else if (btn == buttonKey)
    {
        // turn off buzzer
        Serial.println("Key button clicked");
        buzzerOn = false;
    }
}


// these clicks can be short (0.5s~) or long (and can be measured in any time more than 0.2s?)
void longClick(Button2& btn)
{
    unsigned int timeBtn = btn.wasPressedFor();

    if (btn == buttonUp)
    {
        if (timeBtn > BUTTON_LONG_PRESS)
        {
            Serial.println("Up button long-pressed");
            displayDate();
        }
        else
        {
            Serial.println("Up button short-pressed");
            increaseBrightness();
        }
    }
    else if (btn == buttonDown)
    {
        if (timeBtn > BUTTON_LONG_PRESS)
        {
            Serial.println("Down button long-pressed");
            displayAlarms();
        }
        else
        {
            Serial.println("Down button short-pressed");
            decreaseBrightness();
        }
    }
    else if (btn == buttonSet)
    {
        if (timeBtn > BUTTON_LONG_PRESS)
        {
            Serial.println("Set button long-pressed");
            if (hotspotState == HOTSPOT_CLOSED)
            {
                Serial.print("Starting AP at time: ");
                Serial.println(currentTime);
                displayScrollingText("AP on");
                WiFiMode_t newMode = (hasNetworkConnection ? WIFI_AP_STA : WIFI_AP);
                WiFi.mode(newMode);
                unsigned long startTime = millis();
                while ((WiFi.getMode() != newMode)  && ((millis() - startTime) < 1200))
                {
                    yield();
                }
                WiFi.softAP(config.getDeviceName());
                hotspotState = HOTSPOT_OPEN;
                hotspotOpenTime = millis();
            }
            else
            {
                hotspotOpenTime = millis() - HOTSPOT_OPEN_TIME;
            }
        }
        else
        {
            Serial.println("Set button short-pressed");
            displayIpAddress();
        }
    }
    else if (btn == buttonKey)
    {
        if (timeBtn > BUTTON_LONG_PRESS)
        {
            // turn off buzzer
            Serial.println("Key button long-pressed");
            buzzerOn = false;
        }
        else
        {
            // turn off buzzer
            Serial.println("Key button short-pressed");
            buzzerOn = false;
        }
    }
}

// test the display
void displayTest()
{
    Serial.println("displayTest");

    setDisplayBrightness(MAX_BRIGHTNESS, false);

    for (uint8_t i = 0; i < 3; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            Disp4Seg.setSegments((j == 0) ? 0x7F : 0xFF, j);
            last_AM_PM_brightness = MAX_BRIGHTNESS;
            analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
        }
        delay(600);

        for (uint8_t j = 0; j < 4; j++)
        {
            Disp4Seg.setSegments(0x00, j);
            last_AM_PM_brightness = 0;
            analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
        }
        delay(300);
    }
}

// display a time value
void displayTime(uint8_t hourValue, uint8_t minuteValue, uint8_t secondValue)
{
    // the four time digits
    uint8_t digit0;
    uint8_t digit1;
    uint8_t digit2;
    uint8_t digit3;

    // display the hour digits, converting from 24 hour time to 12 hour time if needed
    uint8_t temp = hourValue;
    switch (config.getDisplayModeNumber())
    {
        // 12 hour display modes
        case TIME_MODE_12_SOLID_COLON:
        case TIME_MODE_12_SOLID_COLON_AM_LED:
        case TIME_MODE_12_SOLID_COLON_PM_LED:
        case TIME_MODE_12_BLINKING_COLON_ALWAYS:
        case TIME_MODE_12_BLINKING_COLON_AM:
        case TIME_MODE_12_BLINKING_COLON_PM:
            if (temp == 0)
            {
                temp = 12;
            }
            if (temp >= 13)
            {
                temp -= 12;
            }
            digit0 = temp / 10;
            digit1 = temp % 10;
            if (digit0 == 0)
            {
                Disp4Seg.setSegments(0, 0);
            }
            else
            {
                Disp4Seg.setDisplayDigit(digit0, 0);
            }
            break;

        // else must be 24 hour mode
        default:
        case TIME_MODE_24_SOLID_COLON:
        case TIME_MODE_24_BLINKING_COLON:
            digit0 = hourValue / 10;
            digit1 = hourValue % 10;
            Disp4Seg.setDisplayDigit(digit0, 0);
            break;
    }
    Disp4Seg.setDisplayDigit(digit1, 1);

    // handle colon
    bool colonOn = true;
    switch (config.getDisplayModeNumber())
    {
        case TIME_MODE_24_BLINKING_COLON:
        case TIME_MODE_12_BLINKING_COLON_ALWAYS:
            if ((secondValue & 0x01) != 0)
            {
                colonOn = false;
            }
            break;

        case TIME_MODE_12_BLINKING_COLON_AM:
            if ((hourValue < 12)  &&  ((secondValue & 0x01) != 0))
            {
                colonOn = false;
            }
            break;

        case TIME_MODE_12_BLINKING_COLON_PM:
            if ((hourValue >= 12)  &&  ((secondValue & 0x01) != 0))
            {
                colonOn = false;
            }
            break;
    }

    // display minute digits
    digit2 = minuteValue / 10;
    digit3 = minuteValue % 10;
    Disp4Seg.setDisplayDigit(digit2, 2, colonOn);
    Disp4Seg.setDisplayDigit(digit3, 3, colonOn);

    // handle external AM/PM LED on GPIO4
    switch (config.getDisplayModeNumber())
    {
        case TIME_MODE_12_SOLID_COLON_AM_LED:
            if (hourValue < 12)
            {
                if (last_AM_PM_brightness != currentBrightness)
                {
                    last_AM_PM_brightness = currentBrightness;
                    analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
                }
            }
            else
            {
                if (last_AM_PM_brightness != 0)
                {
                    last_AM_PM_brightness = 0;
                    analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
                }
            }
            break;

        case TIME_MODE_12_SOLID_COLON_PM_LED:
            if (hourValue >= 12)
            {
                if (last_AM_PM_brightness != currentBrightness)
                {
                    last_AM_PM_brightness = currentBrightness;
                    analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
                }
            }
            else
            {
                if (last_AM_PM_brightness != 0)
                {
                    last_AM_PM_brightness = 0;
                    analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
                }
            }
            break;

        default:
            if (last_AM_PM_brightness != 0)
            {
                last_AM_PM_brightness = 0;
                analogWrite(AM_PM_LED_pin, AM_PM_values[last_AM_PM_brightness]);
            }
            break;
    }
}


void checkHotspotTimeout()
{
    switch (hotspotState)
    {
    case HOTSPOT_CLOSED:
        break;

    case HOTSPOT_OPEN:
        if ((millis() - hotspotOpenTime) >= HOTSPOT_OPEN_TIME)
        {
            Serial.print("Shutting down hotspot at time: ");
            Serial.println(currentTime);
            displayScrollingText("AP OFF");
            hasNetworkConnection ? WiFi.mode(WIFI_STA) : WiFi.mode(WIFI_OFF);
            hotspotState = HOTSPOT_CLOSED;
        }
    }
}


void beep(int note, int duration)
{
    tone(BUZZER_PIN, note, duration);
    delay(duration);
}


void imperialMarchAlarm()
{
    beep(NOTE_A4, 500);
    beep(NOTE_A4, 500);
    beep(NOTE_A4, 500);
    beep(NOTE_F4, 350);
    beep(NOTE_C5, 150);
    beep(NOTE_A4, 500);
    beep(NOTE_F4, 350);
    beep(NOTE_C5, 150);
    beep(NOTE_A4, 650);

    delay(500);

    beep(NOTE_E5, 500);
    beep(NOTE_E5, 500);
    beep(NOTE_E5, 500);
    beep(NOTE_F5, 350);
    beep(NOTE_C5, 150);
    beep(NOTE_GS4, 500);
    beep(NOTE_F4, 350);
    beep(NOTE_C5, 150);
    beep(NOTE_A4, 650);

    delay(500);

    beep(NOTE_A5, 500);
    beep(NOTE_A4, 300);
    beep(NOTE_A4, 150);
    beep(NOTE_A5, 500);
    beep(NOTE_GS5, 325);
    beep(NOTE_G5, 175);
    beep(NOTE_FS5, 125);
    beep(NOTE_F5, 125);
    beep(NOTE_FS5, 250);

    delay(325);

    beep(NOTE_AS4, 250);
    beep(NOTE_DS5, 500);
    beep(NOTE_D5, 325);
    beep(NOTE_CS5, 175);
    beep(NOTE_C5, 125);
    beep(NOTE_AS4, 125);
    beep(NOTE_C5, 250);

    delay(350);
}


class AlarmTune
{
    public:
        AlarmTune(int* melody, int melodyLength, int tempo, int buzzer)
        {
            this->melody = melody;
            this->melodyLength = melodyLength;
            this->tempo = tempo;
            this->buzzer = buzzer;
        }

        void play()
        {
            // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
            // there are two values per note (pitch and duration), so for each note there are four bytes
            int notes = melodyLength / 2;

            // this calculates the duration of a whole note in ms
            int wholenote = (60000 * 4) / tempo;

            int divider = 0, noteDuration = 0;

            // iterate over the notes of the melody.
            // Remember, the array is twice the number of notes (notes + durations)
            for (int noteIndex = 0; noteIndex < notes * 2; noteIndex = noteIndex + 2)
            {

                // calculates the duration of each note
                divider = melody[noteIndex + 1];
                if (divider > 0)
                {
                    // regular note, just proceed
                    noteDuration = (wholenote) / divider;
                } else if (divider < 0)
                {
                    // dotted notes are represented with negative durations!!
                    noteDuration = (wholenote) / abs(divider);
                    noteDuration *= 1.5;        // increases the duration in half for dotted notes
                }

                // we only play the note for 90% of the duration, leaving 10% as a pause
                tone(buzzer, melody[noteIndex], noteDuration * 0.9);
                yield();

                // Wait for the specied duration before playing the next note.
                delay(noteDuration);

                // stop the waveform generation before the next note.
                noTone(buzzer);
                yield();
            }
        }
    private:
        int tempo;
        int buzzer;
        int melodyLength;
        int* melody;
};


int melody[] =
{

    // Pink Panther theme
    // Score available at https://musescore.com/benedictsong/the-pink-panther
    // Theme by Masato Nakamura, arranged by Teddy Mason

    REST,2, REST,4, REST,8, NOTE_DS4,8,
    NOTE_E4,-4, REST,8, NOTE_FS4,8, NOTE_G4,-4, REST,8, NOTE_DS4,8,
    NOTE_E4,-8, NOTE_FS4,8,  NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_E4,8, NOTE_G4,-8, NOTE_B4,8,
    NOTE_AS4,2, NOTE_A4,-16, NOTE_G4,-16, NOTE_E4,-16, NOTE_D4,-16,
    NOTE_E4,2, REST,4, REST,8, NOTE_DS4,4,

    NOTE_E4,-4, REST,8, NOTE_FS4,8, NOTE_G4,-4, REST,8, NOTE_DS4,8,
    NOTE_E4,-8, NOTE_FS4,8,  NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_G4,8, NOTE_B4,-8, NOTE_E5,8,
    NOTE_DS5,1,
    NOTE_D5,2, REST,4, REST,8, NOTE_DS4,8,
    NOTE_E4,-4, REST,8, NOTE_FS4,8, NOTE_G4,-4, REST,8, NOTE_DS4,8,
    NOTE_E4,-8, NOTE_FS4,8,  NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_E4,8, NOTE_G4,-8, NOTE_B4,8,

    NOTE_AS4,2, NOTE_A4,-16, NOTE_G4,-16, NOTE_E4,-16, NOTE_D4,-16,
    NOTE_E4,-4, REST,4,
    REST,4, NOTE_E5,-8, NOTE_D5,8, NOTE_B4,-8, NOTE_A4,8, NOTE_G4,-8, NOTE_E4,-8,
    NOTE_AS4,16, NOTE_A4,-8, NOTE_AS4,16, NOTE_A4,-8, NOTE_AS4,16, NOTE_A4,-8, NOTE_AS4,16, NOTE_A4,-8,
    NOTE_G4,-16, NOTE_E4,-16, NOTE_D4,-16, NOTE_E4,16, NOTE_E4,16, NOTE_E4,2,

};

AlarmTune alarmSound(melody, sizeof(melody) / sizeof(melody[0]), 120, BUZZER_PIN);


void soundAlarm(int alarmSoundNumber)
{
    switch (alarmSoundNumber)
    {
        case ALARM_SOUND_BUZZER:
            buzzerOn = true;
            buzzerStartTime = millis();
            break;

        case ALARM_SOUND_IMPERIAL_MARCH:
            imperialMarchAlarm();
            break;

        case ALARM_SOUND_PINK_PANTHER:
            alarmSound.play();
            break;
    }
}
