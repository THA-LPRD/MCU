#include <ArduinoJson.h>
#include <fstream>
#include <string>
#include "Config.h"
#include "Log.h"

Config::~Config() {
    SaveConfig();
}

void Config::LoadDefaultConfig() {
    Log::Debug("Loading default config");
    SetOperatingMode("Standalone");
    SetWiFiSSID("ESP32-Access-Point");
    SetWiFiPassword("THA-LPRD-2024");
}

void Config::LoadConfig() {
    const char* filename = GetInstance().m_FileName.c_str();
    Log::Debug("Loading config from file.");
    std::ifstream file(filename);

    if (!file) {
        Log::Warning("Failed to open config file for reading - loading default config");
        LoadDefaultConfig();
        SaveConfig();
        return;
    }

    JsonDocument doc;

    if (deserializeJson(doc, file) != DeserializationError::Ok) {
        Log::Error("Failed to parse config file");
        file.close();
        return;
    }

    const char* data = doc["OperatingMode"];
    SetOperatingMode(data);
    data = doc["WiFiSSID"];
    SetWiFiSSID(data);
    data = doc["WiFiPassword"];
    SetWiFiPassword(data);

    file.close();
    return;
}

void Config::SaveConfig() {
    Log::Debug("Saving config to file");
    std::ofstream file(GetInstance().m_FileName);
    if (!file) {
        Log::Error("Failed to open config file for writing");
        return;
    }

    JsonDocument doc;
    doc["OperatingMode"] = GetOperatingMode();
    doc["WiFiSSID"] = GetWiFiSSID();
    doc["WiFiPassword"] = GetWiFiPassword();

    serializeJson(doc, file);
    file.close();
}
