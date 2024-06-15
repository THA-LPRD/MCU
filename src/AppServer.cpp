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

    // Fetch newest Image from Server

    std::ofstream file;
    std::string path = "/upload/";
    
    Log::Debug("Fetching image from server");
    HTTPClient http;
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
            printf("%c", data);
        }
        file.close();
        Log::Info("Image downloaded successfully");
    }
    else {
        Log::Error("Failed to fetch image, HTTP code: %d", httpCode);
    }
    http.end();
    
    Log::Debug("Displaying image");

    m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(path),
        EPDL::GetWidth(),
        EPDL::GetHeight(),
        3));

    EPDL::BeginFrame();
    EPDL::DrawImage(m_ImageHandle, 0, 0);
    EPDL::SwapBuffers();
    EPDL::EndFrame();

    // Get newest Config from Server
    // WiFiClient client;

    // String readRequest = "GET /channels/" + channelID + "/fields/" + fieldNumber + ".json?results=" + numberOfResults + " HTTP/1.1\r\n" + "Host: " + host + "\r\n"
    //     + "Connection: close\r\n\r\n";

    // if (!client.connect(host, httpPort)) {
    //     return;
    // }

    // client.print(readRequest);
    // readResponse(&client);

    // Get newest Image from Server



    // Show Image

    // Sleep

    // m_Server.AddAPIUploadImg([this](std::string_view filePath) {
    //     Log::Debug("Current image path: %s", m_ImagePath.c_str());
    //     if (m_ImagePath != "") {
    //         Log::Debug("Removing previous image: %s", MCU::Filesystem::GetPath(m_ImagePath).c_str());
    //         EPDL::DeleteImage(m_ImageHandle);
    //         if (m_ImagePath != filePath) {
    //             MCU::Filesystem::rm(m_ImagePath);
    //         }
    //     }
    //     if (!MCU::Filesystem::exists(filePath)) {
    //         Log::Error("File not found: %s", MCU::Filesystem::GetPath(filePath).c_str());
    //         m_ImagePath = "";
    //         return;
    //     }
    //     m_ImagePath = filePath;
    //     m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(filePath),
    //         EPDL::GetWidth(),
    //         EPDL::GetHeight(),
    //         3));
    //     m_ProcessImage = true;
    //     });



    DisconnectWiFi();
    return true;
}

void AppServer::Run() {
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