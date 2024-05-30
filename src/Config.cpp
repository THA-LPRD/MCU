#include <ArduinoJson.h>
#include <fstream>
#include <string>
#include "Config.h"
#include "Log.h"

namespace Config
{
    namespace
    { // Private members
        static const char* m_FileName = "/littlefs/config.json";
    } // namespace

    void LoadDefaultConfig() {
        Log::Debug("Loading default config");
        SetOperatingMode("Standalone");
        SetWiFiSSID("ESP32-Access-Point");
        SetWiFiPassword("THA-LPRD-2024");
        SetDisplayDriver("WS_7IN3F");
    }

    void LoadConfig() {
        Log::Debug("Loading config from file.");
        std::ifstream file(m_FileName);

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
        data = doc["DisplayDriver"];
        SetDisplayDriver(data);

        file.close();
        return;
    }

    void SaveConfig() {
        Log::Debug("Saving config to file");
        std::ofstream file(m_FileName);
        if (!file) {
            Log::Error("Failed to open config file for writing");
            return;
        }

        JsonDocument doc;
        doc["OperatingMode"] = GetOperatingMode();
        doc["WiFiSSID"] = GetWiFiSSID();
        doc["WiFiPassword"] = GetWiFiPassword();
        doc["DisplayDriver"] = GetDisplayDriver();

        serializeJson(doc, file);
        file.close();
    }

#define DEFINE_CONFIG_KEY(name, type) \
    namespace { static type m_##name; } \
    void Set##name(const type& value) { m_##name = value; } \
    const type& Get##name() { return m_##name; }

    DEFINE_CONFIG_KEY(OperatingMode, std::string);
    DEFINE_CONFIG_KEY(WiFiSSID, std::string);
    DEFINE_CONFIG_KEY(WiFiPassword, std::string);
    DEFINE_CONFIG_KEY(DisplayDriver, std::string);

#undef DEFINE_CONFIG_KEY
} // namespace Config