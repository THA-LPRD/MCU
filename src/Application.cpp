#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include "Application.h"
#include "Log.h"
#include "Clock.h"
#include "Config.h"

void Application::Init()
{
    Config::LoadConfig();

    SetMode(Config::GetOperatingMode());

    if (!SetupWiFi()) {
        Log::Fatal("Failed to setup WiFi");
        return;
    }
}

bool Application::SetupWiFi() {
    Log::Debug("Setting up WiFi");

    switch (m_Mode) {
    case Mode::STANDALONE:
        WiFi.softAP(Config::GetWiFiSSID().c_str(), Config::GetWiFiPassword().c_str());
        Log::Info("WiFi AP started: %s", Config::GetWiFiSSID().c_str());
        return true;
    case Mode::NETWORK:
        WiFi.begin(Config::GetWiFiSSID().c_str(), Config::GetWiFiPassword().c_str());
        Log::Info("Connecting to WiFi: %s", Config::GetWiFiSSID().c_str());
        Log::Debug("With password: %s", Config::GetWiFiPassword().c_str());
        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Log::Error("Failed to connect to WiFi: %s", Config::GetWiFiSSID().c_str());
            return false;
        }
        Log::Info("Connected to WiFi: %s", Config::GetWiFiSSID().c_str());
        return true;
    default:
        Log::Error("Invalid mode");
        return false;
    }
}

void Application::SetMode(std::string mode) {
    if (mode == "Standalone") {
        SetMode(Mode::STANDALONE);
    }
    else if (mode == "Network") {
        SetMode(Mode::NETWORK);
    }
    else if (mode == "Server") {
        SetMode(Mode::SERVER);
    }
    else {
        Log::Error("Invalid mode");
    }
}
