#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "AppNetwork.h"
#include "Log.h"
#include "Config.h"
#include "EPDL.h"
#include "Filesystem.h"

bool AppNetwork::Init() {
    Log::Debug("Initializing network application");

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    m_Server.Init();
    std::map<std::string, std::string> filesToServe = {
            {"/index.html",         "/www/indexClient.html"},
            {"/style.css",          "/www/style.css"},
            {"/LPRD-Logo.webp",     "/www/LPRD-Logo.webp"},
            {"/html2canvas.min.js", "/www/html2canvas.min.js"},
            {"/utils.js",           "/www/utils.js"}
    };
    m_Server.SetFilesToServe(filesToServe);
    m_Server.AddAPISetOpMode();
    m_Server.AddAPISetWiFiCred();
    m_Server.AddAPIUploadImg([this](std::string_view filePath) {
        if (m_ImagePath != "") {
            EPDL::DeleteImage(m_ImageHandle);
            MCU::Filesystem::rm(m_ImagePath);
        }
        m_ImagePath = filePath;
        m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(filePath,
                                                                            EPDL::GetWidth(),
                                                                            EPDL::GetHeight(),
                                                                            3));
        m_ProcessImage = true;
    });

    return true;
}

bool AppNetwork::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.begin(Config::Get(Config::Key::WiFiSSID).c_str(), Config::Get(Config::Key::WiFiPassword).c_str());
    Log::Info("Connecting to WiFi: %s", Config::Get(Config::Key::WiFiSSID).c_str());
    Log::Debug("With password: %s", Config::Get(Config::Key::WiFiPassword).c_str());
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Log::Error("Failed to connect to WiFi: %s", Config::Get(Config::Key::WiFiSSID).c_str());
        return false;
    }
    Log::Info("Connected to WiFi: %s", Config::Get(Config::Key::WiFiSSID).c_str());
    Log::Info("IP address: %s", WiFi.localIP().toString().c_str());

    if (!MDNS.begin("esp32")) {
        Log::Warning("Failed to start mDNS responder");
    }
    else {
        Log::Info("mDNS responder started");
    }
    return true;
}