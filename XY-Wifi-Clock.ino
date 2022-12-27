// Distributed under AGPL v3 license. See license.txt

// Need to install the WiFiManager library by tzapu (not the other ones)
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson

#include <ESP8266WiFi.h>

// Uses ESP Scheduler library from: https://github.com/nrwiersma/ESP8266Scheduler
#include <Arduino.h>
#include <Scheduler.h>
#include <Task.h>
#include <LeanTask.h>

#include "UpdateTimeTask.h"
UpdateTimeTask* updateTimeTask;

#include "NtpUpdateTask.h"
NtpUpdateTask* ntpUpdateTask;

const int BUTTON_UP = 10;
const int BUTTON_DOWN = 9;
const int BUTTON_SET = 16;

#include "pitches.h"

// TM1650 Digital Display
#include <Wire.h>
#include "TM1650.h"

TM1650 Disp4Seg;

// I2C pins
const int SCL_PIN = 12;
const int SDA_PIN = 13;

const int BUZZER_PIN = 5;

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

// Main setup of clock
void setup() {
    Serial.begin(115200);

    // Just for debugging
    //delay(10000);

    Serial.println("Starting XY-Clock");

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    pinMode(BUZZER_PIN, OUTPUT);

    // pinMode(BUTTON_UP, INPUT);
    // pinMode(BUTTON_DOWN, INPUT);
    // pinMode(BUTTON_SET, INPUT);

    Disp4Seg.Init();
    Disp4Seg.SetBrightness(displayBrightness);
    Disp4Seg.DisplayON();

    //wifiManager.setConfigPortalBlocking(false);
    //wifiManager.setSaveParamsCallback(saveWifiManagerParamsCallback);

    // Just for testing
    //wifiManager.resetSettings();

    displayWaiting();
    //soundAlarm();

    hasNetworkConnection = wifiManager.autoConnect("XY-Clock");

    if (!hasNetworkConnection) {
        wifiManager.startConfigPortal();
        Serial.println("Failed to connect to Wifi Network");
    } else {
        Serial.println("Connected to Wifi Network");

        currentClockState = DisplayingTime;

        // Setup and star the update time task
        updateTimeTask = new UpdateTimeTask(true, 1000);
        updateTimeTask->setTimeZone("BST");
        Scheduler.start(updateTimeTask);

        // NTP server sync task
        ntpUpdateTask = new NtpUpdateTask(true, 1000);
        Scheduler.start(ntpUpdateTask);

        Scheduler.begin();
    }
}

// Called once the Wifi Manager saves the settings
void saveWifiManagerParamsCallback() {
    Serial.println("Save Wifi Params");

    currentClockState = DisplayingTime;
}

// Sets the bottom dot states
void setBottomDots(bool dot0, bool dot1, bool dot2, bool dot3) {
    Disp4Seg.SetDot(0, dot0);
    Disp4Seg.SetDot(1, dot1);
    Disp4Seg.SetDot(2, dot2);
    Disp4Seg.SetDot(3, dot3);
}

// Main loop
void loop() {
    //checkButtons();

    switch (currentClockState) {
        case WaitingForWifi:
            Disp4Seg.WriteNum(8888);
            break;

        case WaitingForNtp:
            Disp4Seg.WriteNum(1111);
            break;

        case NtpUpdateFailure:
            Disp4Seg.WriteNum(9999);
            Disp4Seg.ColonOFF();
            setBottomDots(true, true, false, false);
            break;

        case DisplayingTime:
            displayTime();
            break;
    }
}

// Check the button states and perform the necessary actions
// void checkButtons() {
//     int buttonUpState = digitalRead(BUTTON_UP);
//     int buttonDownState = digitalRead(BUTTON_DOWN);
//     int buttonSetState = digitalRead(BUTTON_SET);

//     if (buttonUpState == HIGH) {
//         displayBrightness++;
//         if (displayBrightness > 5) displayBrightness = 5;

//         Disp4Seg.SetBrightness(displayBrightness);
//     }

//     if (buttonDownState == HIGH) {
//         displayBrightness--;
//         if (displayBrightness < 1) displayBrightness = 1;

//         Disp4Seg.SetBrightness(displayBrightness);
//     }

//     // Using the set button to reset the Wifi Settings is only for testing purposes
//     if (buttonSetState == HIGH) {
//         wifiManager.resetSettings();
//     }
// }

// Displays a spinner
void displayWaiting() {
  Serial.println("displayWaiting");

  for (uint8_t k = 0; k < 3; k++) {
    for (uint8_t i = 0; i < 4; i++) {
      for (uint8_t j = 1; j < 0x40; j = j << 1) {
        Disp4Seg.SendDigit(j, i);
        delay(50);
      }

      Disp4Seg.SendDigit(0, i);
    }
  }
}


// Get the time and convert it to the current timezone. Display it on the segment display
void displayTime() {
    // Display the current time
    Disp4Seg.WriteNum(updateTimeTask->displayNumber);
    Disp4Seg.ColonON();
}

void beep(int note, int duration) {
    tone(BUZZER_PIN, note, duration);
    delay(duration);
}

void soundAlarm() {
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
