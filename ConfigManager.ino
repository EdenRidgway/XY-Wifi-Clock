#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <FS.h>

bool isConfigLoaded = false;
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

void loadSettings() {
    Serial.println("loadSettings");

    ensureSpiffsStarted();

    File configFile = SPIFFS.open("/config.json", "r");

    if (!configFile) {
        config.setDeviceName("XY-Clock");
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
    config.setDeviceName(json["deviceName"]);
    config.setTimezone(json["timezone"]);
    
    // Set defaults
    if (!config.getDeviceName()) {
        config.setDeviceName("XY-Clock");
    }

    if (!config.getTimezone()) {
        config.setTimezone("BST");
    }
    
    if (!json["alarms"]) return;

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

// Converts a config to JSON
DynamicJsonDocument convertConfigToJson() {
    Serial.println("convertConfigToJson");

    DynamicJsonDocument doc(2048);

    doc["deviceName"] = config.getDeviceName();
    doc["timezone"] = config.getTimezone();

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
