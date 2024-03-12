// Requires ESPAsyncTCP

//#include <ESPAsyncTCP.h>
#include <ESP8266WebServer.h>

void startWebServer() {
    Serial.println("Starting Web server");

    server.on("/", handleRoot);
    server.on("/timezones.json", handleGetTimezonesJson);
    server.on("/displayModes.json", handleGetDisplayModesJson);
    server.on("/dateFormats.json", handleGetDateFormatsJson);
    server.on("/favicon.ico", handleGetFavicon);
    server.on("/alarmSounds.json", handleGetAlarmSoundsJson);
    server.onNotFound(handleNotFound);
    server.on("/config", handleConfigJson);

    server.begin();
}

// Convert the file extension to the MIME type
String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    else if (filename.endsWith(".json")) return "application/json";
    return "text/plain";
}

// Handles reading the file from SPIFFS
bool handleFileRead(String path) { // send the right file to the client (if it exists)
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file

    String contentType = getContentType(path);             // Get the MIME type
    String pathWithGz = path + ".gz";

    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
        if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
            path += ".gz";                                         // Use the compressed verion

        File file = SPIFFS.open(path, "r");                    // Open the file
        size_t sent = server.streamFile(file, contentType);    // Send it to the client
        file.close();                                          // Close the file again
        Serial.println(String("Sent file: ") + path);
        return true;
    }

    Serial.println(String("File Not Found: ") + path);   // If the file doesn't exist, return false
    return false;
}

void handleRoot() {
    Serial.println("Web handleRoot");

    // turn off buzzer
    buzzerOn = false;

    if (!handleFileRead("/Config.html")) {
        server.send(404, "text/plain", "404: Not Found");
    }

    if (!handleFileRead("/timezones.json")) {
        server.send(404, "text/plain", "404: Not Found");
    }

    if (!handleFileRead("/displayModes.json")) {
        server.send(404, "text/plain", "404: Not Found");
    }

    if (!handleFileRead("/dateFormats.json")) {
        server.send(404, "text/plain", "404: Not Found");
    }

    if (!handleFileRead("/alarmSounds.json")) {
        server.send(404, "text/plain", "404: Not Found");
    }
}

void handleConfigJson() {

    if (server.method() == HTTP_GET) {
        handleGetConfigJson();
        return;
    }

    if (server.method() == HTTP_POST) {
        handlePostConfigJson();
        return;
    }

    server.send(405, "text/plain", "Method Not Allowed");
    return;
}

void handlePostConfigJson() {
    Serial.println("Web POST handleConfigJson");

    String previousDeviceName     = config.getDeviceName();
    String previousTimezone       = config.getTimezone();
    String previousDisplayMode    = config.getDisplayMode();
    String previousDateFormat     = config.getDateFormat();
    String previousAlarmSound     = config.getAlarmSound();
    bool   previousAutoBrightness = config.getAutoBrightnessEnable();

    DynamicJsonDocument json(2048);

    deserializeJson(json, server.arg("plain"));
    loadSettingsFromJson(json);

    // Write it out to the serial console
    serializeJson(json, Serial);
    Serial.println();

    saveSettings();

    String deviceName     = config.getDeviceName();
    String timezone       = config.getTimezone();
    String displayMode    = config.getDisplayMode();
    String dateFormat     = config.getDateFormat();
    String alarmSound     = config.getAlarmSound();
    bool   autoBrightness = config.getAutoBrightnessEnable();

    server.send(200, "text/plain");

    bool needReset = false;

    if (!deviceName.equalsIgnoreCase(previousDeviceName))
    {
        Serial.print("Device Name changed to: ");
        Serial.println(deviceName);
        needReset = true;
    }

    if (!timezone.equalsIgnoreCase(previousTimezone))
    {
        Serial.print("Timezone changed to: ");
        Serial.println(timezone);
        needReset = true;
    }

    if (!displayMode.equalsIgnoreCase(previousDisplayMode))
    {
        Serial.print("Display Mode changed to: ");
        Serial.println(displayMode);
    }


    if (!dateFormat.equalsIgnoreCase(previousDateFormat))
    {
        Serial.print("Date Format changed to: ");
        Serial.println(dateFormat);
    }

    if (!alarmSound.equalsIgnoreCase(previousAlarmSound))
    {
        Serial.print("Alarm Sound changed to: ");
        Serial.println(alarmSound);
    }

    if (autoBrightness != previousAutoBrightness)
    {
        Serial.print("Auto brightness changed to: ");
        Serial.println(autoBrightness ? "enabled" : "disabled");
    }

    if (needReset)
    {
        Serial.println("Config change(s) will trigger a device restart...");
        ESP.restart();
        delay(1000);
    }
}

void handleGetConfigJson() {
    Serial.println("Web GET handleConfigJson");

    DynamicJsonDocument doc = convertConfigToJson();

    String buffer;
    serializeJson(doc, buffer);
    server.send(200, "application/json", buffer);
}

void handleGetTimezonesJson() {
    Serial.println("Web GET handleGetTimezonesJson");
    handleFileRead("/timezones.json");
}

void handleGetDisplayModesJson() {
    Serial.println("Web GET handleGetDisplayModesJson");
    handleFileRead("/displayModes.json");
}

void handleGetDateFormatsJson() {
    Serial.println("Web GET handleGetDateFormatsJson");
    handleFileRead("/dateFormats.json");
}

void handleGetFavicon() {
    Serial.println("Web GET handleGetFavicon");
    handleFileRead("/favicon.ico");
}

void handleGetAlarmSoundsJson() {
    Serial.println("Web GET handleGetAlarmSoundsJson");
    handleFileRead("/alarmSounds.json");
}

void handleNotFound() {
    Serial.println("Web handleNotFound");

    String message = "Not Found Error\n\n";
    message += "URI: " + server.uri();
    message += "\n\n Method: " + (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\n\n Arguments: "+ server.args();
    message += "\n";

    server.send(404, "text/plain", message);
}
