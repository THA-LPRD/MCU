#include <Arduino.h>
#include <WiFi.h>
#include "AppFailsafe.h"
#include "Log.h"
#include "Config.h"

#define FAILSAFE_SSID "failsafe"
#define FAILSAFE_PASSWORD "sendhelp"


bool AppFailsafe::Init() {
    Log::Debug("Initializing failsafe application");

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    m_Server.Init();

    return true;
}

bool AppFailsafe::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.softAP(FAILSAFE_SSID, FAILSAFE_PASSWORD);
    Log::Info("WiFi AP started: %s", FAILSAFE_SSID);
    Log::Info("IP address: %s", WiFi.softAPIP().toString().c_str());

    return true;
}