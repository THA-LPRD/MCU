#include <ArduinoJson.h>
#include <fstream>
#include <unordered_map>
#include "Config.h"
#include "Log.h"
#include <Filesystem.h>

namespace Config
{
    namespace
    { // Private members
        constexpr char m_FileName[] = "/config.json";
        std::unordered_map<Key, std::string> m_Config;
    } // namespace

    std::string GetDefault(Key key) {
        if (m_Items.find(key) == m_Items.end()) {
            Log::Error("[Config] Key not found in Config");
            return "";
        }
        return m_Items.at(key).default_value;
    }

    void LoadDefault() {
        Log::Debug("[Config] Loading default config");
        for (const auto &item: m_Items) {
            m_Config[item.first] = item.second.default_value;
        }
    }

    void Load() {
        Log::Debug("Loading config from file.");
        std::ifstream file(MCU::Filesystem::GetPath(m_FileName));

        if (!file) {
            Log::Warning("[Config] Failed to open config file for reading - loading default config");
            LoadDefault();
            Save();
            return;
        }

        JsonDocument doc;

        if (deserializeJson(doc, file) != DeserializationError::Ok) {
            Log::Error("[Config] Failed to parse config file");
            file.close();
            return;
        }

        for (JsonObject::iterator it = doc.as<JsonObject>().begin(); it != doc.as<JsonObject>().end(); ++it) {
            m_Config[m_ReverseItems.at(it->key().c_str())] = it->value().as<std::string>();
        }

        file.close();
    }

    void Save() {
        Log::Debug("Saving config to file");
        std::ofstream file(MCU::Filesystem::GetPath(m_FileName));
        if (!file) {
            Log::Error("[Config] Failed to open config file for writing");
            return;
        }

        JsonDocument doc;
        for (const auto &pair: m_Config) {
            doc[m_Items.at(pair.first).name] = pair.second;
        }

        serializeJson(doc, file);
        file.close();
    }

    bool Set(Key key, std::string_view value) {
        if (m_Items.find(key) == m_Items.end()) {
            Log::Error("[Config] Key not found in Config");
            return false;
        }
        if (!m_Items.at(key).validator(value)) {
            Log::Error("[Config] Value for key %s is invalid", m_Items.at(key).name);
            return false;
        }
        m_Config[key] = value;
        return true;
    }

    std::string Get(Key key) {
        if (m_Config.find(key) == m_Config.end()) {
            Log::Error("[Config] Key not found in Config");
            return "";
        }
        return m_Config.at(key);
    }
} // namespace Config