#include <Arduino.h>
#include <WiFi.h>
#include "AppDefault.h"
#include "Log.h"
#include "Config.h"


bool AppDefault::Init() {
    Log::Debug("Initializing default application");
    Config::LoadDefaultConfig();

    SetupWiFi();

    m_Server.Init();

    return true;
}

bool AppDefault::SetupWiFi() {
    Log::Debug("Setting up Default WiFi Access Point");

    WiFi.softAP(Config::GetWiFiSSID().c_str(), Config::GetWiFiPassword().c_str());
    Log::Info("WiFi AP started: %s", Config::GetWiFiSSID().c_str());
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    Log::Debug("Setting up DNS server");
    m_DNSServer.start(53, "*", WiFi.softAPIP());
    return true;
}