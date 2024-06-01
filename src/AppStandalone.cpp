#include <Arduino.h>
#include <WiFi.h>
#include "AppStandalone.h"
#include "Log.h"
#include "Config.h"
#include "EPDL.h"

bool AppStandalone::Init() {
    Log::Debug("Initializing standalone application");

    SetupWiFi();

    m_Server.Init();
    return true;
}

void AppStandalone::Run() {
    m_DNSServer.processNextRequest();
}

bool AppStandalone::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.softAP(Config::GetWiFiSSID().c_str(), Config::GetWiFiPassword().c_str());
    Log::Info("WiFi AP started: %s", Config::GetWiFiSSID().c_str());
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    Log::Debug("Setting up DNS server");
    m_DNSServer.start(53, "*", WiFi.softAPIP());
    return true;
}