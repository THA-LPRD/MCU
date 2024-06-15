
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

    // Fetch newest Config from Server
    Log::Debug("Fetching config from server");

    http.begin("http://192.168.10.150:3000/");

    // Fetch newest Image from Server
    Log::Debug("Fetching image from server");

    std::ofstream file;
    std::string path = "/upload/";
    
    http.begin("http://192.168.10.150:3000/uploads/image.png"); // Replace with the actual URL of the image
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Log::Debug("Location: %s", http.getLocation().c_str());
        path += "/image.png";
        std::string pathNative = MCU::Filesystem::GetPath(path);
        file.open(pathNative, std::ios::binary | std::ios::trunc);

        WiFiClient* stream = http.getStreamPtr();

        // File file = SPIFFS.open("/image.png", FILE_WRITE); // Replace with the desired file path
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

    // Get newest Image from Server



    // Show Image

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
