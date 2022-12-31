// Requires ESPAsyncTCP

//#include <ESPAsyncTCP.h>
#include <ESP8266WebServer.h>

void startWebServer() {
    Serial.println("Starting web server");

    server.on("/", handleRoot);
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

    if (!handleFileRead("/Config.html")) {
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

    DynamicJsonDocument json(2048);

    deserializeJson(json, server.arg("plain"));
    loadSettingsFromJson(json);

    saveSettings();
    
    server.send(200, "text/plain");
}

void handleGetConfigJson() {
    Serial.println("Web GET handleConfigJson");

    DynamicJsonDocument doc = convertConfigToJson();

    String buffer;
    serializeJson(doc, buffer);
    server.send(200, "application/json", buffer);
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
