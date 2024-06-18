#include <Arduino.h>
#include <WiFi.h>
#include "AppStandalone.h"
#include "Log.h"
#include "Config.h"
#include "EPDL.h"
#include <Filesystem.h>
#include "MCU.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>
AppStandalone::~AppStandalone() {
    DestroyWiFi();
}

bool AppStandalone::Init() {
    Log::Debug("Initializing standalone application");
    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    if (!m_Server.Init(Config::Get(Config::Key::HTTPS) == "true")) {
        Log::Fatal("Failed to initialize server");
        DestroyWiFi();
        return false;
    }

    std::map<std::string, std::string> filesToServe = {
            {"/index.html",               "/www/index.html"},
            {"/settings.html",            "/www/settings.html"},
            {"/style.css",                "/www/style.css"},
            {"/LPRD-Logo.webp",           "/www/LPRD-Logo.webp"},
            {"/icons8-settings-25-w.png", "/www/icons8-settings-25-w.png"},
            {"/html2canvas.min.js",       "/www/html2canvas.min.js"},
            {"/utils.js",                 "/www/utils.js"},
            {"/script.js",                "/www/script.js"},
            {"/layouts",                  "/www/layouts"},
            {"/layouts/fonts",            "/www/layouts/fonts"},
    };
    m_Server.SetFilesToServe(filesToServe);
    m_Server.AddAPISetOpMode();
    m_Server.AddAPISetWiFiCred();
    m_Server.AddAPIUploadImg([this](std::string_view filePath) {
        Log::Debug("Current image path: %s", m_ImagePath.c_str());
        if (!m_ImagePath.empty()) {
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
        m_ProcessImage = true;
    });
    m_Server.AddAPIGetDisplayModule();
    m_Server.AddAPISetDisplayModule();
    m_Server.AddAPIGetOpMode();
    m_Server.AddAPIGetDisplayWidth();
    m_Server.AddAPIGetDisplayHeight();
    m_Server.AddAPIRestart();
    m_Server.AddAPISetLogLevel();
    m_Server.AddAPIGetLogLevel();
    m_Server.AddAPISetHTTPAuth();
    m_Server.AddAPISetHTTPS();
    m_Server.AddAPIGetHTTPS();
    m_Server.AddAPIUploadHTTPSKey();
    m_Server.AddAPIUploadHTTPSCert();
    m_Server.EnableHTTPAuth(Config::Get(Config::Key::HTTPUsername), Config::Get(Config::Key::HTTPPassword));

    return true;
}

void AppStandalone::Run() {
    m_DNSServer.processNextRequest();
    if (m_ProcessImage) {
        m_ProcessImage = false;
        Log::Debug("Current image path: %s", m_ImagePath.c_str());
        std::unique_ptr<EPDL::ImageData> imageData = std::make_unique<EPDL::ImageData>(MCU::Filesystem::GetPath(
                m_ImagePath));
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
        DestroyWiFi();
        rtc_gpio_pulldown_dis((gpio_num_t) 2);
        rtc_gpio_pullup_en((gpio_num_t) 2);
        esp_sleep_enable_ext1_wakeup(1ULL << 2, ESP_EXT1_WAKEUP_ANY_LOW);

        MCU::DeepSleep();
    }
}

bool AppStandalone::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(Config::Get(Config::Key::WiFiSSID).c_str(), Config::Get(Config::Key::WiFiPassword).c_str())) {
        Log::Error("Failed to start WiFi AP");
        return false;
    }
    Log::Info("WiFi AP started: %s", Config::Get(Config::Key::WiFiSSID).c_str());
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    Log::Debug("Setting up DNS server");
    if (m_DNSServer.start(53, "*", WiFi.softAPIP())) {
        Log::Info("DNS server started");
    }
    else {
        Log::Warning("Failed to start DNS server");
    }
    return true;
}

void AppStandalone::DestroyWiFi() {
    Log::Debug("Destroying WiFi");
    m_DNSServer.stop();
    WiFi.softAPdisconnect();
    WiFi.disconnect(true, true);
}
