// Distributed under AGPL v3 license. See license.txt

// Need to install the WiFiManager library by tzapu (not the other ones) https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiManager.h>   // if using Arduino IDE, click here to search: http://librarymanager#WiFiManager_ESPx_tzapu
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson // if using Arduino IDE, click here to search: http://librarymanager#ArduinoJson
#include <ArduinoOTA.h>  // https://github.com/jandrassy/ArduinoOTA // if using Arduino IDE, click here to search: http://librarymanager#ArduinoOTA

//#include <ESP8266WiFi.h>

#include "pitches.h"

#include <Adafruit_GFX.h> // Adafruit_GFX https://github.com/adafruit/Adafruit-GFX-Library // if using Arduino IDE, click here to search: http://librarymanager#Adafruit_GFX_graphics_core_library
#include <TM1650.h> // TM1650 Digital Display https://github.com/maxint-rd/TM16xx // if using Arduino IDE, click here to search: http://librarymanager#TM16xx_LEDs_and_Buttons
#include <TM16xxMatrixGFX.h>
#include <TM16xxDisplay.h>

// I2C pins
const int SCL_PIN = 12;
const int SDA_PIN = 13;

#define NUM_DIGITS 4

TM1650 Disp4Seg(SDA_PIN, SCL_PIN);
TM16xxDisplay display(&Disp4Seg, NUM_DIGITS);

#define MATRIX_NUMCOLUMNS 5
#define MATRIX_NUMROWS 3
TM16xxMatrixGFX matrix(&Disp4Seg, MATRIX_NUMCOLUMNS, MATRIX_NUMROWS);    // TM16xx object, columns, rows#

const int BUZZER_PIN = 5;

#include <WiFiUdp.h>
#include <NTPClient.h> // NTPClient by Fabrice Weinberg https://github.com/arduino-libraries/NTPClient // if using Arduino IDE, click here to search: http://librarymanager#NTPClient
#include <time.h> // Time library  https://github.com/PaulStoffregen/Time  // if using Arduino IDE, click here to search: http://librarymanager#Timekeeping_functionality_for_Arduino

//extern bool readyForNtp = false;

// Track the state of the clock setup
enum ClockState {
  WaitingForWifi = 1,
  WaitingForNtp = 2,
  NtpUpdateFailure = 3,
  DisplayingTime = 4
};

#include <Button2.h> // Button2 by Lennart Hennigs https://github.com/LennartHennigs/Button2 // if using Arduino IDE, click here to search: http://librarymanager#Button2
#define buttonUpPin 10
#define buttonDownPin 9
#define buttonSetPin 16
Button2 buttonUp, buttonDown, buttonSet;

ClockState currentClockState = WaitingForWifi;

bool hasNetworkConnection = false;
WiFiManager wifiManager;

struct tm* currentTimeinfo;
uint16_t currentDisplayTime = 0;
uint8_t displayHour = 0;
uint8_t displayMinute = 0;
uint8_t currentWeekDay = 0;
bool isPm = false;
bool setHotspot = false;
unsigned long setHotspotTime = 0;
unsigned long setHotspotDuration = 1000*60*5; // Hotspot open for 5 mins
uint8_t brightness = 4;
uint8_t displayBrightness = 4;
uint8_t previousAlarmBrightness = 4;
int8_t myBrightness = 0;

class Alarm {
    protected:
        uint8_t alarmHour;
        uint8_t alarmMinute;
        uint8_t lastHour = -1;
        uint8_t lastMinute = -1;
        uint8_t daysOfWeek = 0;
    
    public:
        uint8_t getHour() {
            return alarmHour;
        }

        void setHour(uint8_t value) {
            alarmHour = value;
        }

        uint8_t getMinute() {
            return alarmMinute;
        }

        void setMinute(uint8_t value) {
            alarmMinute = value;
        }
        
        // Whether or not it is for the day of the week
        bool isForDayOfWeek(uint8_t day) {
            return (daysOfWeek & (1UL << day));   
        }

        void clearDaysOfWeek() {
            daysOfWeek = 0;
        }

        void enableDayOfWeek(uint8_t day) {
            daysOfWeek |= (1UL << day);
        }

        // Determines if the alarm has triggered
        bool hasAlarmTriggered(uint8_t hour, uint8_t minute, uint8_t day) {
            if (daysOfWeek == 0) {
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
                if (hour == alarmHour && minute == alarmMinute && isDayOfWeekMatch)
                {
                    Serial.println("Alarm triggered");
                    return true;
                }
            }

            return false;
        }
};

class BrightnessAlarm: public Alarm {
    private:
        uint8_t alarmBrightness = 4;

    public:
        BrightnessAlarm(uint8_t hour = 0, uint8_t minute = 0, uint8_t brightness = 4) {
            alarmHour = hour;
            alarmMinute = minute;
            alarmBrightness = brightness;
            daysOfWeek = 255;
        }

        void setBrightness(uint8_t value) {
            if (value > 7) value = 7;
            if (value < 1) value = 1;
            alarmBrightness = value;
        }
    
        uint8_t getBrightness() {
            return alarmBrightness;
        }
};

class Config {
    private:
        String deviceName;
        String timezone;

    public:
        Alarm alarms[6]= { Alarm(), Alarm(), Alarm(), Alarm(), Alarm(), Alarm() };
        bool twelvehourMode;
        
        BrightnessAlarm* dayBrightnessAlarm;
        BrightnessAlarm* nightBrightnessAlarm;

        Config() {
            dayBrightnessAlarm = new BrightnessAlarm(6, 0, 5);
            nightBrightnessAlarm = new BrightnessAlarm(20, 0, 3);
        }

        void setDeviceName(String value) {
            deviceName = value;
        }

        void setTimezone(String value) {
            timezone = value;
        }

        void settwelvehourMode(bool value) {
            twelvehourMode = value;
        }

        String getDeviceName() {
            if (deviceName == NULL || deviceName == "") {
                deviceName = "XY-Clock";
            }
            
            return deviceName;
        }

        String getTimezone() {
            return timezone;
        }

        bool gettwelvehourMode() {
             return twelvehourMode;
        }
};

Config config;

bool isSpiffsStarted = false;

void ensureSpiffsStarted() {
    if (isSpiffsStarted) return;

    if (!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    Serial.println("SPIFFS started");
    isSpiffsStarted = true;
}

class Timezones {
    public:
        /* Get timezone posix string if it is a valid timezone */
        static String getTimezonePosixText(String olsonTimezone) {
            ensureSpiffsStarted();
            
            File timeZonesFile = SPIFFS.open("/timezones.json", "r");

            DynamicJsonDocument doc(17000);
            deserializeJson(doc, timeZonesFile);
            timeZonesFile.close();

            JsonObject documentRoot = doc.as<JsonObject>();
            String posixText = documentRoot[olsonTimezone];
            
            Serial.print("Checking timezone: ");
            Serial.print(olsonTimezone);
            Serial.print(". Posix text found: ");
            Serial.println(posixText);

            doc.garbageCollect();
            timeZonesFile.close();

            if (posixText == NULL) {
                posixText = "GMT0";
            }

            return posixText;
        }

        /* Is valid timezone name */
        static bool isValidTimezone(String olsonTimezone) {
            ensureSpiffsStarted();
            
            File timeZonesFile = SPIFFS.open("/timezones.json", "r");

            DynamicJsonDocument doc(17000);
            deserializeJson(doc, timeZonesFile);
            timeZonesFile.close();

            JsonObject documentRoot = doc.as<JsonObject>();
            String posixText = documentRoot[olsonTimezone];
            
            timeZonesFile.close();
            doc.garbageCollect();

            if (posixText == NULL) {
                return false;
            }

            return true;
        }
};

#include <ESP8266mDNS.h>

ESP8266WebServer server(80);

String posixTimezoneText;

bool isDisplayingScrollingText = false;

// Main setup of clock
void setup() {
    Serial.begin(115200);

    Serial.println("Starting XY-Clock");
    Serial.println("Built on " __DATE__ " at " __TIME__);
    
    pinMode(BUZZER_PIN, OUTPUT);

    //Button2 Setup Handlers
    buttonUp.begin(buttonUpPin);
    buttonUp.setClickHandler(click);
    buttonUp.setLongClickHandler(longClick);
    buttonDown.begin(buttonDownPin);
    buttonDown.setClickHandler(click);
    buttonDown.setLongClickHandler(longClick);
    buttonSet.begin(buttonSetPin);
    buttonSet.setClickHandler(click);
    buttonSet.setLongClickHandler(longClick);

    Serial.println("Setting up TM1650 Display");
    
    wifiManager.setDebugOutput(true);
    //wifiManager.setConfigPortalBlocking(false);
    //wifiManager.setSaveParamsCallback(saveWifiManagerParamsCallback);

    // Just for testing
    //wifiManager.resetSettings();

    displayWaiting();

    Serial.println("Starting Wifi Manager");
    delay(50);
    hasNetworkConnection = wifiManager.autoConnect("XY-Clock");

    if (!hasNetworkConnection) {
        wifiManager.startConfigPortal();
        Serial.println("Failed to connect to Wifi Network");
    } else {
        Serial.println("Connected to Wifi Network");

        currentClockState = DisplayingTime;

        loadSettings();

        String timezone = config.getTimezone();
        
        if (!Timezones::isValidTimezone(timezone)) {
            timezone = "Etc/GMT";
        }
        
        Serial.print("Device name: ");
        Serial.println(config.getDeviceName());
        Serial.print("Timezone: ");
        Serial.println(timezone);

        config.setTimezone(timezone);

        posixTimezoneText = Timezones::getTimezonePosixText(timezone);

        Serial.print("Timezone posix text: ");
        Serial.println(posixTimezoneText);

        setenv("TZ", posixTimezoneText.c_str(), 0);  // https://github.com/nayarsystems/posix_tz_db 

        // NTP Time sync once every 6 hours
        configTime(6*3600, 0, "pool.ntp.org", "time.nist.gov");
        Serial.print("Getting NTP time ");
        
        while (!time(nullptr)) {
          Serial.println("*");
          delay(250);  
        }

        bool twelvehourMode = config.gettwelvehourMode();
        if (not (twelvehourMode == true || twelvehourMode == false)) {
            twelvehourMode = false;
        }
        Serial.print("12 Hour Mode: ");
        Serial.println(twelvehourMode);
        config.settwelvehourMode(twelvehourMode);

        updateDisplayBrightness();
        
         // Start the mDNS responder
        Serial.println("Setup MDNS ");
        if (MDNS.begin(config.getDeviceName())) {
            Serial.println("mDNS responder started");
        } else {
            Serial.println("Error setting up MDNS responder!");
        }

        ArduinoOTA.setHostname((config.getDeviceName()).c_str()); // weird: this decides the mDNS name
        ArduinoOTA.onStart([]() {
            Serial.println("OTA Start");
            displaySomething("ota");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA End");
            displaySomething("done");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Update Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
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

/* Displays Scrolling Text */
void displayScrollingText(String text) {
    isDisplayingScrollingText = true;
    
    int endPos = 0-text.length()-1;

    Serial.print("Displaying text ");
    Serial.print(text.length());
    Serial.print(" characters long: ");
    Serial.print(text);
    Serial.print(" End cursor pos: ");
    Serial.println(endPos);

    for (int cursorPosition = 0; cursorPosition > endPos; cursorPosition--) {
        Serial.print("Cursor Position: ");
        Serial.println(cursorPosition);

        display.setCursor(cursorPosition);
        display.println(text);
        delay(500);
        display.clear();
    }
    
    isDisplayingScrollingText = false;
}

void displayIpAddress() {
    isDisplayingScrollingText = true;
    
    display.println("IP");
    delay(1500);
        
    IPAddress ipAddress = WiFi.localIP();
        
    for (int i=0; i < 4; i++) {
        display.println(ipAddress[i]);
        delay(1500);
        yield();
    }
    
    isDisplayingScrollingText = false;
}

void displaySomething(String text) {
    isDisplayingScrollingText = true;

    display.println(text);
    delay(750);
    yield();

    isDisplayingScrollingText = false;
}

void updateDisplayBrightness() {    
    uint16_t dayBrightnessTime = (int)config.dayBrightnessAlarm->getHour() * 100 + config.dayBrightnessAlarm->getMinute();
    uint16_t nightBrightnessTime = (int)config.nightBrightnessAlarm->getHour() * 100 + config.nightBrightnessAlarm->getMinute();

    uint8_t alarmBrightness = 4;
    bool isDayMatch = false;

    // Use the day brightness if we are in range otherwise switch to night brightness
    if (currentDisplayTime >= dayBrightnessTime && currentDisplayTime < nightBrightnessTime) {
        isDayMatch = true;
        alarmBrightness = config.dayBrightnessAlarm->getBrightness();
    } else {
        alarmBrightness = config.nightBrightnessAlarm->getBrightness();
    }

    if (previousAlarmBrightness != alarmBrightness)  {
        myBrightness = 0;
    }

    if ((myBrightness + alarmBrightness) < 1)  {
        myBrightness = 1 - alarmBrightness;
    }
    if ((myBrightness + alarmBrightness) > 7) {
        myBrightness = 7 - alarmBrightness;
    }

    brightness = (alarmBrightness + myBrightness);

    if (displayBrightness == brightness) {
        return;
    }

    displayBrightness = brightness;
    previousAlarmBrightness = alarmBrightness;

    Serial.print("Setting brightness to ");
    Serial.println(displayBrightness);

    matrix.setIntensity(displayBrightness);
    matrix.fillScreen(LOW);
    matrix.write();
    // uncomment if you want to display the brightness level on the screen:
    //Disp4Seg.setDisplayToDecNumber(displayBrightness);
    //delay(300);
}

void increaseBrightness() {
    myBrightness = myBrightness + 1;
    Serial.print("Button brightness up");
    Serial.println(myBrightness);
}

void decreaseBrightness() {
    myBrightness = myBrightness - 1;
    Serial.print("Button brightness down");
    Serial.println(myBrightness);
}

// Called once the Wifi Manager saves the settings
void saveWifiManagerParamsCallback() {
    Serial.println("Save Wifi Params");

    currentClockState = DisplayingTime;
}

// Checks the alarms
void checkAlarms() {
    for (auto& alarm : config.alarms) {
        if (alarm.hasAlarmTriggered(displayHour, displayMinute, currentWeekDay)) {
            //yield();
            soundAlarm();
        }
    }
}

/* Updates the display time */
void updateDisplayTime() {
    time_t rawtime; 
    time(&rawtime); 

    currentTimeinfo = localtime(&rawtime); 

    displayHour = currentTimeinfo->tm_hour;
    displayMinute = currentTimeinfo->tm_min;
    currentWeekDay = currentTimeinfo->tm_wday;
    // Set Sunday appropriately
    if (currentWeekDay == 0) currentWeekDay = 7;

    // convert to 12 hour time if twelvehourMode is on
    isPm = false;
    bool twelvehourMode = config.gettwelvehourMode();
    if (twelvehourMode == true) {
        if (displayHour == 12) {
            isPm = true;
        }
        if (displayHour == 0) {
            displayHour = 12;
        }
        if (displayHour >= 13) {
            displayHour = (displayHour - 12);
            isPm = true;
        }
    }

    currentDisplayTime = displayHour * 100 + displayMinute;
}

// Main loop
void loop() {    
    server.handleClient();
    yield();

    MDNS.update();
    yield();

    updateDisplayTime();
    yield();

    switch (currentClockState) {
        case WaitingForWifi:
            displaySomething("net");
            break;

        case WaitingForNtp:
            displaySomething("ntp");
            break;

        case NtpUpdateFailure:
            displaySomething("-ntp");
            break;

        case DisplayingTime:
            displayTime();
            yield();
            break;
    }
    
    checkAlarms();
    yield();

    buttonUp.loop();
    buttonDown.loop();
    buttonSet.loop();
    yield();

    checkHotspotOpen();
    yield();

    updateDisplayBrightness();
    yield();
    
    ArduinoOTA.handle();
    yield();
}

void click(Button2& btn) { // these clicks can be pretty short! (so handling same as short-presses)
    if (btn == buttonUp) {
        Serial.println("Up button clicked");
        increaseBrightness();
    }
    else if (btn == buttonDown) {
        Serial.println("Down button clicked");
        decreaseBrightness();
    }
    else if (btn == buttonSet) {
        Serial.println("Set button clicked");
        displayIpAddress();
    }
}

void longClick(Button2& btn) { // these clicks can be short (0.5s~) or long (and can be measured in any time more than 0.2s?)
    unsigned int timeBtn = btn.wasPressedFor();
    if (btn == buttonUp) {
        if (timeBtn > 3000) {
            Serial.println("Up button long-pressed");
            displaySomething("UP--");
        } else {
            Serial.println("Up button short-pressed");
            increaseBrightness();
        }
    }
    else if (btn == buttonDown) {
        if (timeBtn > 3000) {
            Serial.println("Down button long-pressed");
            displaySomething("dn--");
        } else {
            Serial.println("Down button short-pressed");
            decreaseBrightness();
        }
    }
    else if (btn == buttonSet) {
        if (timeBtn > 3000) { // hold the set button for longer than 3s to turn the hotspot on/off
            Serial.print("Set button long-pressed");
            if (setHotspot == false) {
                setHotspotTime = millis();
            }
            else {
                setHotspotTime = (millis() - setHotspotDuration);
            }
        } else {
            Serial.print("Set button short-pressed");
            displayIpAddress();
        }
    }
}

// Displays a spinner
void displayWaiting() {
    Serial.println("displayWaiting");
    for (uint8_t k = 0; k < 3; k++) {
        for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t j = 1; j < 0x40; j = j << 1) {
                Disp4Seg.setSegments(j, i);
                delay(50);
                yield();
            }
            Disp4Seg.setSegments(0, i);
        }
    }
}

// Get the time and convert it to the current timezone. Display it on the segment display
void displayTime() {
    if (isDisplayingScrollingText) return;
    
    uint8_t digit0 = (currentDisplayTime / 1000) % 10;
    uint8_t digit1 = (currentDisplayTime / 100) % 10;
    uint8_t digit2 = (currentDisplayTime / 10) % 10;
    uint8_t digit3 = currentDisplayTime % 10;
    uint8_t currentSec = currentTimeinfo->tm_sec;
    bool colonOn = false;

    // make the colon blink
    if ((currentSec & 0x01) == 0) {
        colonOn = true;
    }
    
    // if twelvehourMode is on and the first digit is zero, make it blank
    bool twelveHourMode = config.gettwelvehourMode();
        
    if ((twelveHourMode == true) && (digit0 == 0)) {
        Disp4Seg.setSegments(0, 0);
    } else {
        Disp4Seg.setDisplayDigit(digit0, 0);
    }
    
    Disp4Seg.setDisplayDigit(digit1, 1, isPm && twelveHourMode);
    // Add the colon on to the last 2 digits
    Disp4Seg.setDisplayDigit(digit2, 2, colonOn);
    Disp4Seg.setDisplayDigit(digit3, 3, colonOn);

    yield();
}

void checkHotspotOpen() {
    // the Hotspot should stay open for the time specified by setHotspotDuration... set stopHotspotTime when the Set Button was pressed
    if (setHotspotTime != 0) {
        if ((millis() >= (setHotspotTime)) && (setHotspot == false)) {
            Serial.print("Setting up hotspot at time:");
            Serial.print(currentDisplayTime);
            displayScrollingText("HOTSPOT ON");
            WiFi.mode(WIFI_AP_STA);
            delay(2000);
            WiFi.softAP(config.getDeviceName());
            setHotspot = true;
        }
        if ((millis() >= (setHotspotTime + setHotspotDuration)) && (setHotspot == true)) {
            Serial.print("Shutting down hotspot at time:");
            Serial.print(currentDisplayTime);
            displayScrollingText("HOTSPOT OFF");
            WiFi.mode(WIFI_STA);
            setHotspotTime = 0;
            setHotspot = false;
        }
    }
}

void beep(int note, int duration) {
    tone(BUZZER_PIN, note, duration);
    delay(duration);
}

void imperialMarchAlarm() {
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
    yield();

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
    yield();

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
    yield();

    beep(NOTE_AS4, 250);
    beep(NOTE_DS5, 500);
    beep(NOTE_D5, 325);
    beep(NOTE_CS5, 175);
    beep(NOTE_C5, 125);
    beep(NOTE_AS4, 125);
    beep(NOTE_C5, 250);

    delay(350);
    yield();
}

class AlarmTune {
    public:
        AlarmTune(int* melody, int melodyLength, int tempo, int buzzer) {
            this->melody = melody;
            this->melodyLength = melodyLength;
            this->tempo = tempo;
            this->buzzer = buzzer;
        }

        void play() {
            // sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
            // there are two values per note (pitch and duration), so for each note there are four bytes
            int notes = melodyLength / 2;

            // this calculates the duration of a whole note in ms
            int wholenote = (60000 * 4) / tempo;

            int divider = 0, noteDuration = 0;

            // iterate over the notes of the melody.
            // Remember, the array is twice the number of notes (notes + durations)
            for (int noteIndex = 0; noteIndex < notes * 2; noteIndex = noteIndex + 2) {

                // calculates the duration of each note
                divider = melody[noteIndex + 1];
                if (divider > 0) {
                    // regular note, just proceed
                    noteDuration = (wholenote) / divider;
                } else if (divider < 0) {
                    // dotted notes are represented with negative durations!!
                    noteDuration = (wholenote) / abs(divider);
                    noteDuration *= 1.5; // increases the duration in half for dotted notes
                }

                // we only play the note for 90% of the duration, leaving 10% as a pause
                tone(buzzer, melody[noteIndex], noteDuration * 0.9);
                yield();

                // Wait for the specief duration before playing the next note.
                delay(noteDuration);
                yield();

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

int melody[] = {

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

void soundAlarm() {
    //imperialMarchAlarm();
    alarmSound.play();
}
