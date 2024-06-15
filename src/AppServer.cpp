
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "AppServer.h"
#include "Log.h"
#include "Config.h"
#include "EPDL.h"
#include "Filesystem.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <fstream>

bool AppServer::Init() {
    Log::Debug("Initializing server application");

    if (Config::Get(Config::Key::ServerURL).empty()) {
        Log::Error("Server URL not set in config");
        return false;
    }

    if(!SetupWiFi()) {
        Log::Error("Failed to setup WiFi");
        return false;
    }

    HTTPClient http;

    // Check if display is already registered on server
    Log::Debug("Checking if display is already registered on server");

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    if (CheckRegistered(mac)) {
        Log::Info("Display is already registered on server");
        String imageURLPath = GetNewConfig(mac);
        DisplayImage(GetNewImage(imageURLPath));
    } else {
        Log::Info("Display is not yet registered on server");
        RegisterDisplay(mac);
    }

    // Sleep

    DisconnectWiFi();
    return true;
}

void AppServer::Run() {
    // Not used in Servermode. 
}

bool AppServer::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    //WiFi.begin(Config::Get(Config::Key::WiFiSSID).c_str(), Config::Get(Config::Key::WiFiPassword).c_str());
    WiFi.begin("Bahnrallye", "Bahnrallye23");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Log::Debug("Connecting to WiFi...");
    }

    Log::Info("WiFi connected to: %s", Config::Get(Config::Key::WiFiSSID).c_str());
    Log::Info("IP address: %s", WiFi.localIP().toString().c_str());

    /* Log::Debug("Setting up DNS server");
    m_DNSServer.start(53, "*", WiFi.softAPIP()); */
    return true;
}

bool AppServer::DisconnectWiFi() {
    Log::Debug("Closing WiFi");
    WiFi.disconnect();
    return true;
}

bool AppServer::CheckRegistered(uint8_t *mac) {
    HTTPClient http;
    char checkRegisteredURL[100];
    sprintf(checkRegisteredURL, "%S/api/v1/displays/%02X%02X%02X%02X%02X%02X", Config::Get(Config::Key::ServerURL).c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    http.begin(checkRegisteredURL);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        Log::Info("Display is already registered on server");
        http.end();
        return true;

    } else if (httpCode == HTTP_CODE_NOT_FOUND) {
        Log::Info("Display not registered on server");
        http.end();
        return false;
    } else {
        Log::Error("Failed to check if display is registered, HTTP code: %d", httpCode);
        // TODO: Better Error handling
        http.end();
        return false;
    }
}

bool AppServer::RegisterDisplay(uint8_t *mac) {
    // Register as new display on server
    Log::Debug("Registering as new display on server");

    HTTPClient http;

    // Print ESP32 MAC address
    
    char registerURL[65];
    sprintf(registerURL, "%S/api/v1/displays/register/%02X%02X%02X%02X%02X%02X", Config::Get(Config::Key::ServerURL).c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Create payload from JSON
    DynamicJsonDocument newDisplayPayload(1024);
    newDisplayPayload["friendly_name"] = "Display1";
    newDisplayPayload["width"] = EPDL::GetWidth();
    newDisplayPayload["height"] = EPDL::GetHeight();

    // Serialize JSON to string
    String newDisplayPayloadString;
    serializeJson(newDisplayPayload, newDisplayPayloadString);

    http.begin(registerURL);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Content-Length", String(newDisplayPayloadString.length()));
    // http.setPayload(payloadString);
 
    int httpCode = http.PUT(newDisplayPayloadString);

    if (httpCode == HTTP_CODE_OK) {
        Log::Info("Registered as new display on server");
        http.end();
        return true;
    } else {
        Log::Error("Failed to register as new display, HTTP code: %d", httpCode);
        http.end();
        return false;
    }    
}

String AppServer::GetNewConfig(uint8_t *mac) {
// Get newest config from server
    Log::Debug("Fetching config from server");
    HTTPClient http;
    String imageURLPath;
    char configURL[65];
    sprintf(configURL, "%S/api/v1/displays/config/%02X%02X%02X%02X%02X%02X", Config::Get(Config::Key::ServerURL).c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    http.begin(configURL);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        Log::Info("Config fetched successfully");
        String configPayloadString = http.getString();
        Log::Debug("Config: %s", configPayloadString.c_str());
        // Parse JSON
        DynamicJsonDocument configPayload(1024);
        DeserializationError error = deserializeJson(configPayload, configPayloadString);
        if (error) {
            Log::Error("Failed to parse JSON config");
        } else {
            // Set config values
            imageURLPath = configPayload["file_path"].as<String>();
            // Config::Set(Config::Key::OperatingMode, configPayload["operating_mode"].as<String>());
            
        }
        // Set config values

    }
    else {
        Log::Error("Failed to fetch config, HTTP code: %d", httpCode);
        
    }
    http.end();
    return imageURLPath;
}

std::string AppServer::GetNewImage(String imageURLPath) {
    // Fetch newest Image from Server
    Log::Debug("Fetching image from server");
    HTTPClient http;

    std::ofstream file;
    std::string path = "/upload/";

    char imageURL[100];
    sprintf(imageURL, "%S%S", Config::Get(Config::Key::ServerURL).c_str(), imageURLPath.c_str());
    
    http.begin(imageURL); // Replace with the actual URL of the image
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        path += "/image.png";
        // TODO: Check if file exists and delete it
        // path += imageURLPath.c_str();
        std::string pathNative = MCU::Filesystem::GetPath(path);
        file.open(pathNative, std::ios::binary | std::ios::trunc);

        WiFiClient* stream = http.getStreamPtr();

        if (!file) {
            Log::Error("Failed to open file for writing");
            return "";
        }
        while (stream->available()) {
            char data = stream->read();
            file.write(&data, sizeof(data));
            // printf("%c", data);
        }
        file.close();
        Log::Info("Image downloaded successfully");
    }
    else {
        Log::Error("Failed to fetch image, HTTP code: %d", httpCode);
    }
    http.end();
    return path;
}

void AppServer::DisplayImage(std::string path) {
    Log::Debug("Displaying image");

    std::unique_ptr<EPDL::ImageData> imageData = std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(path));
    m_ImageHandle = EPDL::CreateImage(std::move(imageData));

    EPDL::BeginFrame();
    EPDL::DrawImage(m_ImageHandle, 0, 0);
    EPDL::SwapBuffers();
    EPDL::EndFrame();
}