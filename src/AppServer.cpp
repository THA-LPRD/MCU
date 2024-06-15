
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

    SetupWiFi();

    HTTPClient http;

    // TODO: Server URL in Config

    // Register as new display on server
    Log::Debug("Registering as new display on server");

    // Print ESP32 MAC address
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    Log::Info("ESP32 MAC address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // Convert MAC address to string
    char registerURL[65];
    sprintf(registerURL, "http://192.168.10.150:3000/api/v1/displays/register/%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

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
    }
    else {
        Log::Error("Failed to register as new display, HTTP code: %d", httpCode);
    }

    http.end();

    // Get newest config from server
    Log::Debug("Fetching config from server");

    String imageURLPath;
    char configURL[65];
    sprintf(configURL, "http://192.168.10.150:3000/api/v1/displays/config/%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

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
            return false;
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

    // Fetch newest Image from Server
    Log::Debug("Fetching image from server");

    std::ofstream file;
    std::string path = "/upload/";

    char imageURL[100];
    sprintf(imageURL, "http://192.168.10.150:3000%S", imageURLPath.c_str());
    
    http.begin(imageURL); // Replace with the actual URL of the image
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Log::Debug("Location: %s", http.getLocation().c_str());
        path += "/image.png";
        std::string pathNative = MCU::Filesystem::GetPath(path);
        file.open(pathNative, std::ios::binary | std::ios::trunc);

        WiFiClient* stream = http.getStreamPtr();

        if (!file) {
            Log::Error("Failed to open file for writing");
            return false;
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
    
    Log::Debug("Displaying image");

    std::unique_ptr<EPDL::ImageData> imageData = std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(path));
    m_ImageHandle = EPDL::CreateImage(std::move(imageData));

    EPDL::BeginFrame();
    EPDL::DrawImage(m_ImageHandle, 0, 0);
    EPDL::SwapBuffers();
    EPDL::EndFrame();

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
