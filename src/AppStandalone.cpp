#include <Arduino.h>
#include <WiFi.h>
#include "AppStandalone.h"
#include "Log.h"
#include "Config.h"

bool AppStandalone::Init() {
    Log::Debug("Initializing standalone application");

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    m_Server.Init();

    return true;
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