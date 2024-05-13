#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "AppNetwork.h"
#include "Log.h"
#include "Config.h"

bool AppNetwork::Init() {
    Log::Debug("Initializing network application");

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return false;
    }

    m_Server.Init(&m_RenderIMG);

    return true;
}

bool AppNetwork::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    WiFi.begin(Config::GetWiFiSSID().c_str(), Config::GetWiFiPassword().c_str());
    Log::Info("Connecting to WiFi: %s", Config::GetWiFiSSID().c_str());
    Log::Debug("With password: %s", Config::GetWiFiPassword().c_str());
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Log::Error("Failed to connect to WiFi: %s", Config::GetWiFiSSID().c_str());
        return false;
    }
    Log::Info("Connected to WiFi: %s", Config::GetWiFiSSID().c_str());
    Log::Info("IP address: %s", WiFi.localIP().toString().c_str());

    if (!MDNS.begin("esp32"))
        Log::Warning("Failed to start mDNS responder");
    else
        Log::Info("mDNS responder started");
    return true;
}