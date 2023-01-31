#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <FS.h>

bool isConfigLoaded = false;

void loadSettings() {
    Serial.println("loadSettings");

    ensureSpiffsStarted();

    File configFile = SPIFFS.open("/config.json", "r");

    if (!configFile) {
        config.setDeviceName("XYClock");
        config.setTimezone("UTC");

        Serial.println("Failed to open config file for reading");
        saveSettings();
        return;
    }

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, configFile);
    configFile.close();

    // Debug output
    serializeJson(doc, Serial);
    Serial.println();

    loadSettingsFromJson(doc);

    Serial.print("Device name: ");
    Serial.println(config.getDeviceName());
    Serial.print("Timezone: ");
    Serial.println(config.getTimezone());

    isConfigLoaded = true;
    Serial.println("loadSettings complete");
}

// Loads the settings from a JSON document
void loadSettingsFromJson(DynamicJsonDocument json) {
    Serial.println("loadSettingsFromJson");
    serializeJson(json, Serial);

    config.setDeviceName(json["deviceName"]);
    config.setTimezone(json["timezone"]);
    config.settwelvehourMode(json["twelvehourMode"].as<bool>());

    // Set defaults
    if (!config.getDeviceName()) {
        config.setDeviceName("XYClock");
    }

    if (!config.getTimezone()) {
        config.setTimezone("Europe/London");
    }

    if (!config.gettwelvehourMode()) {
        config.settwelvehourMode(false);
    }
    
    if (json["dayBrightness"])
    {
        parseBrightnessAlarmConfig(json["dayBrightness"], config.dayBrightnessAlarm);
    }

    if (json["nightBrightness"])
    {
        parseBrightnessAlarmConfig(json["nightBrightness"], config.nightBrightnessAlarm);
    }

    if (json["alarms"])
    {
        auto jsonAlarms = json["alarms"].as<JsonArray>();
        int index = 0;
        for (JsonVariant jsonAlarm : jsonAlarms) {
            config.alarms[index].setHour(jsonAlarm["hour"].as<byte>());
            config.alarms[index].setMinute(jsonAlarm["minute"].as<byte>());
            config.alarms[index].clearDaysOfWeek();

            auto jsonDaysOfWeek = jsonAlarm["daysOfWeek"].as<JsonArray>();
            int dayIndex = 1;

            config.alarms[index].clearDaysOfWeek();

            for (JsonVariant jsonDay : jsonDaysOfWeek) {
                bool isDayOfWeek = jsonDay.as<bool>();
                
                // Serial.print("Day of week ");
                // Serial.print(dayIndex);
                // Serial.print(" is ");
                // Serial.println(isDayOfWeek);

                if (isDayOfWeek) {
                    config.alarms[index].enableDayOfWeek(dayIndex);
                }

                dayIndex++;
            }

            index++;
        }
    }
}

// Set the brightness alarm config
void parseBrightnessAlarmConfig(JsonObject json, BrightnessAlarm* brightnessAlarm) {
    brightnessAlarm->setBrightness(json["brightness"].as<byte>());
    brightnessAlarm->setHour(json["hour"].as<byte>());
    brightnessAlarm->setMinute(json["minute"].as<byte>());
}

// Converts a config to JSON
DynamicJsonDocument convertConfigToJson() {
    Serial.println("convertConfigToJson");

    DynamicJsonDocument doc(2048);

    doc["deviceName"] = config.getDeviceName();
    doc["timezone"] = config.getTimezone();
    doc["twelvehourMode"] = config.gettwelvehourMode();
    
    doc["dayBrightness"] = doc.createNestedObject();
    doc["dayBrightness"]["hour"] = config.dayBrightnessAlarm->getHour();
    doc["dayBrightness"]["minute"] = config.dayBrightnessAlarm->getMinute();
    doc["dayBrightness"]["brightness"] = config.dayBrightnessAlarm->getBrightness();

    doc["nightBrightness"] = doc.createNestedObject();
    doc["nightBrightness"]["hour"] = config.nightBrightnessAlarm->getHour();
    doc["nightBrightness"]["minute"] = config.nightBrightnessAlarm->getMinute();
    doc["nightBrightness"]["brightness"] = config.nightBrightnessAlarm->getBrightness();

    JsonArray alarmsJson = doc.createNestedArray("alarms");

    for (Alarm& alarm : config.alarms) {
        JsonObject alarmJson = alarmsJson.createNestedObject();

        alarmJson["hour"] = alarm.getHour();
        alarmJson["minute"] = alarm.getMinute();
        
        JsonArray daysOfWeek = alarmJson.createNestedArray("daysOfWeek");

        for (byte day = 1; day <= 7; day++) {
            bool isDayOfWeekEnabled = alarm.isForDayOfWeek(day);
            daysOfWeek.add(isDayOfWeekEnabled);
        }
    }

    return doc;
}

void saveSettings() {
    Serial.println("");
    Serial.println("saveSettings");

    ensureSpiffsStarted();
    
    File configFile = SPIFFS.open("/config.json", "w");

    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return;
    }

    DynamicJsonDocument doc = convertConfigToJson();
    serializeJson(doc, configFile);

    // Debug output
    serializeJson(doc, Serial);
    Serial.println();
    
    configFile.close();
    
    Serial.println("Settings saved");
}
