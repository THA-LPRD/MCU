#include <Arduino.h>
#include <WiFi.h>
#include "AppDefault.h"
#include "Log.h"
#include "Config.h"
#include <Filesystem.h>

bool AppDefault::Init() {
    Log::Debug("Initializing default application");
    Config::Set(Config::Key::WiFiSSID, Config::GetDefault(Config::Key::WiFiSSID));

    SetupWiFi();

    m_Server.Init();
    std::map<std::string, std::string> filesToServe = {
            {"/index.html",               "/www/settings.html"},
            {"/style.css",                "/www/style.css"},
            {"/LPRD-Logo.webp",           "/www/LPRD-Logo.webp"},
            {"/icons8-settings-25-w.png", "/www/icons8-settings-25-w.png"},
            {"/utils.js",                 "/www/utils.js"},
    };
    m_Server.SetFilesToServe(filesToServe);
    m_Server.AddAPISetOpMode();
    m_Server.AddAPISetWiFiCred();
    m_Server.AddAPISetServerURL();
    m_Server.AddAPIGetDisplayModule();
    m_Server.AddAPISetDisplayModule();
    m_Server.AddAPIGetOpMode();
    m_Server.AddAPIRestart();
    m_Server.AddAPISetLogLevel();
    m_Server.AddAPIGetLogLevel();
    m_Server.AddAPISetHTTPAuth();
    m_Server.AddAPISetHTTPS();
    m_Server.AddAPIGetHTTPS();
    m_Server.AddAPIUploadHTTPSKey();
    m_Server.AddAPIUploadHTTPSCert();
    m_Server.EnableHTTPAuth(Config::GetDefault(Config::Key::HTTPUsername),
                            Config::GetDefault(Config::Key::HTTPPassword));

    return true;
}

bool AppDefault::SetupWiFi() {
    Log::Debug("Setting up Default WiFi Access Point");

    WiFi.disconnect(true, true);
    WiFi.softAP(Config::GetDefault(Config::Key::WiFiSSID).c_str(),
                Config::GetDefault(Config::Key::WiFiPassword).c_str());
    Log::Info("WiFi AP started: %s", Config::GetDefault(Config::Key::WiFiSSID).c_str());
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    Log::Debug("Setting up DNS server");
    m_DNSServer.start(53, "*", WiFi.softAPIP());
    return true;
}