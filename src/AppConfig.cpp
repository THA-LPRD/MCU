#include <Arduino.h>
#include <WiFi.h>
#include "AppConfig.h"
#include "Log.h"
#include "Config.h"

bool AppConfig::Init() {
    Log::Debug("Initializing config application");

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    m_Server.Init();

    return true;
}

bool AppConfig::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.softAP(Config::GetWiFiSSID().c_str(), Config::GetWiFiPassword().c_str());
    Log::Info("WiFi AP started: %s", WiFi.softAPIP().toString().c_str());
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    return true;
}