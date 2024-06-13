#include <Arduino.h>
#include <WiFi.h>
#include "AppStandalone.h"
#include "Log.h"
#include "Config.h"
#include "EPDL.h"
#include "Filesystem.h"
#include "MCU.h"

bool AppStandalone::Init() {
    Log::Debug("Initializing standalone application");
    SetupWiFi();

    m_Server.Init();
    std::map<std::string, std::string> filesToServe = {
            {"/index.html",               "/www/index.html"},
            {"/settings.html",            "/www/settings.html"},
            {"/style.css",                "/www/style.css"},
            {"/LPRD-Logo.webp",           "/www/LPRD-Logo.webp"},
            {"/icons8-settings-25-w.png", "/www/icons8-settings-25-w.png"},
            {"/html2canvas.min.js",       "/www/html2canvas.min.js"},
            {"/utils.js",                 "/www/utils.js"},
            {"/script.js",                "/www/script.js"},
            {"/layouts",                  "/www/layouts"}
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
        // Does not fit in Stack hence crashes
//        m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(filePath),
//                                                                            EPDL::GetWidth(),
//                                                                            EPDL::GetHeight(),
//                                                                            3));
//        m_ImageHandle = EPDL::CreateImage(std::make_unique<EPDL::ImageData>(filePath,
//                                                                            EPDL::GetWidth(),
//                                                                            EPDL::GetHeight(),
//                                                                            3));
        m_ProcessImage = true;
    });
    m_Server.AddAPIGetDisplayModule();
    m_Server.AddAPISetDisplayModule();
    m_Server.AddAPIGetOpMode();
    m_Server.AddAPIGetDisplayWidth();
    m_Server.AddAPIGetDisplayHeight();
    return true;
}

void AppStandalone::Run() {
    m_DNSServer.processNextRequest();
    if (m_ProcessImage) {
        m_ProcessImage = false;
        Log::Debug("Current image path: %s", m_ImagePath.c_str());
        std::unique_ptr<EPDL::ImageData> imageData = std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(m_ImagePath));
        Log::Debug("Created Image");
        m_ImageHandle = EPDL::CreateImage(std::move(imageData));
        Log::Debug("Got handle: %d", m_ImageHandle);
        EPDL::BeginFrame();
        EPDL::DrawImage(m_ImageHandle, 0, 0);
        EPDL::SwapBuffers();
        EPDL::EndFrame();
        Log::Debug("Ready for next image");
        Log::Debug("Going to sleep");
        MCU::Sleep(1000);
        MCU::DeepSleep();
    }
}

bool AppStandalone::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.softAP(Config::Get(Config::Key::WiFiSSID).c_str(), Config::Get(Config::Key::WiFiPassword).c_str());
    Log::Info("WiFi AP started: %s", Config::Get(Config::Key::WiFiSSID).c_str());
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    Log::Debug("Setting up DNS server");
    m_DNSServer.start(53, "*", WiFi.softAPIP());
    return true;
}