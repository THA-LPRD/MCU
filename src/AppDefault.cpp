#include <Arduino.h>
#include <WiFi.h>
#include "AppDefault.h"
#include "Log.h"
#include "Config.h"


bool AppDefault::Init() {
    Log::Debug("Initializing default application");

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    m_Server.Init();

    return true;
}

bool AppDefault::SetupWiFi() {
    Log::Debug("Setting up Default WiFi Access Point");

    WiFi.softAP(DEFAULT_WIFI_AP_SSID, DEFAULT_WIFI_PASSWORD);
    
    Log::Info("WiFi AP started: %s", DEFAULT_WIFI_AP_SSID);
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    return true;
}