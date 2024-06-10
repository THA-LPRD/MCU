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
            {"/index.html",               "/www/index.html"},
            {"/settings.html",            "/www/settings.html"},
            {"/style.css",                "/www/style.css"},
            {"/LPRD-Logo.webp",           "/www/LPRD-Logo.webp"},
            {"/icons8-settings-25-w.png", "/www/icons8-settings-25-w.png"},
            {"/html2canvas.min.js",       "/www/html2canvas.min.js"},
            {"/utils.js",                 "/www/utils.js"},
            {"/script.js",                "/www/script.js"}
    };
    m_Server.SetFilesToServe(filesToServe);
    m_Server.AddAPISetOpMode();
    m_Server.AddAPISetWiFiCred();
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
        m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(filePath)));
        m_ProcessImage = true;
    });
    m_Server.AddAPIGetDisplayModule();
    m_Server.AddAPISetDisplayModule();
    m_Server.AddAPIGetOpMode();
    m_Server.AddAPIGetDisplayWidth();
    m_Server.AddAPIGetDisplayHeight();

    return true;
}

void AppNetwork::Run() {
    if (m_ProcessImage) {
        m_ProcessImage = false;
        EPDL::BeginFrame();
        EPDL::DrawImage(m_ImageHandle, 0, 0);
        EPDL::SwapBuffers();
        EPDL::EndFrame();
    }
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