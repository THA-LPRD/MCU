#include <Arduino.h>
#include <WiFi.h>
#include "AppServer.h"
#include "Log.h"
#include "Config.h"
#include "EPDL.h"
#include "Filesystem.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

bool AppServer::Init() {
    Log::Debug("Initializing server application");

    SetupWiFi();

    
    
    m_Server.AddAPIUploadImg([this](std::string_view filePath) {
        Log::Debug("Current image path: %s", m_ImagePath.c_str());
        if (m_ImagePath != "") {
            Log::Debug("Removing previous image: %s", MCU::Filesystem::GetPath(m_ImagePath).c_str());
            EPDL::DeleteImage(m_ImageHandle);
            if (m_ImagePath != filePath) {
                MCU::Filesystem::rm(m_ImagePath);
            }
        }
        if (!MCU::Filesystem::exists(filePath)) {
            Log::Error("File not found: %s", MCU::Filesystem::GetPath(filePath).c_str());
            m_ImagePath = "";
            return;
        }
        m_ImagePath = filePath;
        m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(filePath),
                                                                            EPDL::GetWidth(),
                                                                            EPDL::GetHeight(),
                                                                            3));
        m_ProcessImage = true;
    });

    DisconnectWiFi();
    return true;
}

void AppServer::Run() {
}

bool AppServer::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.begin(Config::Get(Config::Key::WiFiSSID).c_str(), Config::Get(Config::Key::WiFiPassword).c_str());
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