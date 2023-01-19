// Distributed under AGPL v3 license. See license.txt

// Need to install the WiFiManager library by tzapu (not the other ones)
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson

//#include <ESP8266WiFi.h>

#include "UpdateTimeTask.h"
UpdateTimeTask updateTimeTask;

const int BUTTON_UP = 10;
const int BUTTON_DOWN = 9;
const int BUTTON_SET = 16;

#include "pitches.h"

// TM1650 Digital Display
// Need to install Adafruit_GFX and TM16xx
#include <Adafruit_GFX.h>
#include <TM1650.h>
#include <TM16xxMatrixGFX.h>

// I2C pins
const int SCL_PIN = 12;
const int SDA_PIN = 13;

TM1650 Disp4Seg(SDA_PIN, SCL_PIN);

#define MATRIX_NUMCOLUMNS 5
#define MATRIX_NUMROWS 3
TM16xxMatrixGFX matrix(&Disp4Seg, MATRIX_NUMCOLUMNS, MATRIX_NUMROWS);    // TM16xx object, columns, rows#

const int BUZZER_PIN = 5;
// Need to install NTPClient by Fabrice Weinberg
// Source for NTP with timezones: https://randomnerdtutorials.com/esp32-ntp-timezones-daylight-saving/
#include <WiFiUdp.h>
#include <NTPClient.h>

// Needs the time library: https://github.com/PaulStoffregen/Time
#include <time.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000 * 30);  // Use the pool.ntp.org an update it every 30 minutes

//extern bool readyForNtp = false;

// Task that runs as a separate "thread" to display the time on the TM1650 Segmented display
class NtpUpdateTask {
    public:
        NtpUpdateTask() {
            
        }
        
        void loop() {
            updateTime();
        }

        void initialise() {
            timeClient.begin();
        }

        void updateTime() {
            if (timeClient.update()) {
                Serial.println("NTP Time updated");

                unsigned long epoch = timeClient.getEpochTime();
                setTime(epoch);
                yield();
            }
        }
};

NtpUpdateTask ntpUpdateTask;

// Track the state of the clock setup
enum ClockState {
  WaitingForWifi = 1,
  WaitingForNtp = 2,
  NtpUpdateFailure = 3,
  DisplayingTime = 4
};

ClockState currentClockState = WaitingForWifi;

bool hasNetworkConnection = false;
WiFiManager wifiManager;

int displayBrightness = 1;

class Alarm {
    protected:
        byte alarmHour;
        byte alarmMinute;
        byte daysOfWeek = 0;
    
        byte lastMinute = 0;

    public:
        byte getHour() {
            return alarmHour;
        }

        void setHour(byte value) {
            alarmHour = value;
        }

        byte getMinute() {
            return alarmMinute;
        }

        void setMinute(byte value) {
            alarmMinute = value;
        }
        
        // Whether or not it is for the day of the week
        bool isForDayOfWeek(byte day) {
            return (daysOfWeek & (1UL << day));   
        }

        void clearDaysOfWeek() {
            daysOfWeek = 0;
        }

        void enableDayOfWeek(byte day) {
            daysOfWeek |= (1UL << day);
        }

        // Determines if the alarm has triggered
        bool hasAlarmTriggered() {
            if (daysOfWeek == 0) {
                return false;
            }
            
            int min = minute();

            bool hasMinutesChanged = (lastMinute != min);
            
            // Update the display time when the time changes
            if (hasMinutesChanged) {
                lastMinute = min;

                tm timeParts = updateTimeTask.getTimeParts();
                int weekDay = timeParts.tm_wday;

                if (timeParts.tm_hour == alarmHour && timeParts.tm_min == alarmMinute && isForDayOfWeek(timeParts.tm_wday))
                {
                    Serial.println("Alarm triggered");
                    return true;
                } 

                return false;
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
        
        BrightnessAlarm dayBrightnessAlarm;
        BrightnessAlarm nightBrightnessAlarm;

        Config() {
            // dayBrightnessAlarm = new BrightnessAlarm(6, 0, 3);
            // nightBrightnessAlarm = new BrightnessAlarm(20, 0, 6);
        }

        void setDeviceName(String value) {
            deviceName = value;
        }

        void setTimezone(String value) {
            timezone = value;
        }

        String getDeviceName() {
            return deviceName;
        }

        String getTimezone() {
            return timezone;
        }
};

Config config;
uint8_t brightness = 4;

#include <ESP8266mDNS.h>

ESP8266WebServer server(80);

// Main setup of clock
void setup() {
    Serial.begin(115200);

    Serial.println("Starting XY-Clock");
    Serial.println("Built on " __DATE__ " at " __TIME__);
    
    pinMode(BUZZER_PIN, OUTPUT);

    //pinMode(BUTTON_UP, INPUT);
    //pinMode(BUTTON_DOWN, INPUT);
    //pinMode(BUTTON_SET, INPUT);

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

        Serial.print("Device name: ");
        Serial.println(config.getDeviceName());
        Serial.print("Timezone: ");
        Serial.println(config.getTimezone());

        Serial.println("Setting timezone");
        updateTimeTask.setTimezone(config.getTimezone());

        // Use a value between 1 and 7 for brightness
        matrix.setIntensity(config.dayBrightnessAlarm.getBrightness());
        matrix.fillScreen(LOW);
        matrix.write();                 // Send the memory bitmap to the display

        Serial.print("Setup MDNS ");

         // Start the mDNS responder        
        if (MDNS.begin(config.getDeviceName())) {
            Serial.println("mDNS responder started");
        } else {
            Serial.println("Error setting up MDNS responder!");
        }

        ntpUpdateTask.initialise();
        ntpUpdateTask.updateTime();

        startWebServer();
    }
}

// Called once the Wifi Manager saves the settings
void saveWifiManagerParamsCallback() {
    Serial.println("Save Wifi Params");

    currentClockState = DisplayingTime;
}

// Checks the alarms
void checkAlarms() {
    for (auto& alarm : config.alarms) {
        if (alarm.hasAlarmTriggered()) {
            //yield();
            soundAlarm();
        }
    }
}

// Main loop
void loop() {    
    server.handleClient();
    yield();

    MDNS.update();
    yield();

    updateTimeTask.loop();
    ntpUpdateTask.loop();
    yield();

    checkAlarms();
    //checkButtons();

    switch (currentClockState) {
        case WaitingForWifi:
            Disp4Seg.setDisplayToDecNumber(8888);
            break;

        case WaitingForNtp:
            Disp4Seg.setDisplayToDecNumber(1111);
            break;

        case NtpUpdateFailure:
            Disp4Seg.setDisplayToDecNumber(9999);
            break;

        case DisplayingTime:
            displayTime();
            yield();
            break;
    }
}

// Check the button states and perform the necessary actions
void checkButtons() {
    int buttonUpState = digitalRead(BUTTON_UP);
    int buttonDownState = digitalRead(BUTTON_DOWN);
    int buttonSetState = digitalRead(BUTTON_SET);

    if (buttonUpState == LOW) {
        brightness++;
        if (brightness > 7) brightness = 7;

        Serial.print("Setting brightness ");
        Serial.println(brightness);
        matrix.setIntensity(brightness);
    }

    if (buttonDownState == LOW) {
        brightness--;
        if (brightness < 1) brightness = 1;

        Serial.print("Setting brightness ");
        Serial.println(brightness);
        matrix.setIntensity(brightness);
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
	uint8_t digit0 = (updateTimeTask.displayNumber / 1000) % 10;
	uint8_t digit1 = (updateTimeTask.displayNumber / 100) % 10;
	uint8_t digit2 = (updateTimeTask.displayNumber / 10) % 10;
	uint8_t digit3 = updateTimeTask.displayNumber % 10;

    Disp4Seg.setDisplayDigit(digit0, 0);
    Disp4Seg.setDisplayDigit(digit1, 1);
    // Add the colon on to the last 2 digits
    Disp4Seg.setDisplayDigit(digit2, 2, true);
    Disp4Seg.setDisplayDigit(digit3, 3, true);
    yield();
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
