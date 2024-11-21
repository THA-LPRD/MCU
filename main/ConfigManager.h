#ifndef LPRD_MCU_CONFIGMANAGER_H
#define LPRD_MCU_CONFIGMANAGER_H

#include <spdlog/spdlog.h>
#include <memory>
#include <string>
#include <fstream>
#include <utility>
#include <ArduinoJson.h>
#include "nvs.h"

class ConfigManager {
public:
    explicit ConfigManager(std::string_view configName)
            :
            m_ConfigName(configName.data()),
            m_NVSNamespace("ConfigManager"),
            m_Document() {

        bool status = LoadDefaults();

        if (!status) {
            spdlog::warn("{} Failed to load configuration defaults, using empty configuration", LOG_TAG);
        }

        if (!LoadFromNVS()) {
            if (!status) {
                spdlog::warn("{} Failed to load configuration from file", LOG_TAG);
            }
            spdlog::warn("{} Failed to load configuration from file, using defaults", LOG_TAG);
        }

        SaveToNVS();
    }

    ~ConfigManager() {
        spdlog::debug("{} Destroying ConfigManager: {}", LOG_TAG, m_ConfigName);
        SaveToNVS();
    }

    bool LoadDefaults() {
        extern const unsigned char config_json_start[] asm("_binary_config_json_start");
        extern const unsigned char config_json_end[] asm("_binary_config_json_end");

        std::string jsonStr((char*) config_json_start, (char*) config_json_end);

        JsonDocument tempDoc;
        DeserializationError error = deserializeJson(tempDoc, jsonStr);
        if (error) {
            spdlog::error("{} Failed to parse configuration: {}", LOG_TAG, error.c_str());
            return false;
        }

        JsonVariant moduleConfig = tempDoc[m_ConfigName];
        if (moduleConfig.isNull()) {
            spdlog::error("{} Configuration for module {} not found", LOG_TAG, m_ConfigName);
            return false;
        }

        error = deserializeJson(m_Document, moduleConfig.as<std::string>());
        if (error) {
            spdlog::error("{} Failed to parse module configuration: {}", LOG_TAG, error.c_str());
            return false;
        }
        return true;
    }

    bool LoadFromNVS() {
        spdlog::debug("{} Loading {} configuration from NVS", LOG_TAG, m_ConfigName);
        nvs_handle_t handle;
        esp_err_t err = nvs_open(m_NVSNamespace.c_str(), NVS_READONLY, &handle);
        if (err != ESP_OK) {
            spdlog::error("{} Error opening NVS handle: {}", LOG_TAG, esp_err_to_name(err));
            return false;
        }

        // Get the size of stored JSON string
        size_t required_size = 0;
        err = nvs_get_str(handle, m_ConfigName.c_str(), nullptr, &required_size);
        if (err != ESP_OK) {
            nvs_close(handle);
            return false;
        }

        std::string jsonStr;
        jsonStr.resize(required_size);
        err = nvs_get_str(handle, m_ConfigName.c_str(), &jsonStr[0], &required_size);
        nvs_close(handle);

        if (err != ESP_OK) {
            spdlog::error("{} Error reading from NVS: {}", LOG_TAG, esp_err_to_name(err));
            return false;
        }

        JsonDocument tempDoc;
        DeserializationError error = deserializeJson(tempDoc, jsonStr);
        if (error) {
            spdlog::error("{} Failed to parse configuration for merging: {}", LOG_TAG, error.c_str());
            return false;
        }

        auto src = tempDoc.as<JsonObjectConst>();
        for (JsonPairConst kvp: src) {
            m_Document[kvp.key()] = kvp.value();
        }

        spdlog::debug("{} Configuration loaded from NVS", LOG_TAG);
        return true;
    }

    bool SaveToNVS() const {
        spdlog::debug("{} Saving {} configuration to NVS", LOG_TAG, m_ConfigName);
        nvs_handle_t handle;
        esp_err_t err = nvs_open(m_NVSNamespace.c_str(), NVS_READWRITE, &handle);
        if (err != ESP_OK) {
            spdlog::error("{} Error opening NVS handle: {}", LOG_TAG, esp_err_to_name(err));
            return false;
        }

        std::string jsonStr;
        serializeJson(m_Document, jsonStr);

        err = nvs_set_str(handle, m_ConfigName.c_str(), jsonStr.c_str());
        if (err != ESP_OK) {
            spdlog::error("{} Error writing to NVS: {}", LOG_TAG, esp_err_to_name(err));
            nvs_close(handle);
            return false;
        }

        err = nvs_commit(handle);
        if (err != ESP_OK) {
            spdlog::error("{} Error committing to NVS: {}", LOG_TAG, esp_err_to_name(err));
            nvs_close(handle);
            return false;
        }

        nvs_close(handle);
        spdlog::debug("{} Configuration saved to NVS", LOG_TAG);
        return true;
    }

    template<typename T>
    T Get(std::string_view key, const T &defaultValue = T()) const {
        if (!m_Document[key].is<T>()) {
            return defaultValue;
        }
        return m_Document[key].as<T>();
    }

    std::string Get(std::string_view key, std::string_view defaultValue = "") const {
        auto variant = m_Document[key];
        if (!variant.is<const char*>()) {
            return {defaultValue.data()};
        }
        return {variant.as<const char*>()};
    }

    std::string Get(std::string_view key, const char* defaultValue = "") const {
        return Get(key, std::string_view(defaultValue));
    }

    std::string Get(std::string_view key, const std::string &defaultValue) const {
        return Get(key, std::string_view(defaultValue));
    }

    std::string Get(std::string_view key, std::string &&defaultValue) const {
        return Get(key, std::string_view(defaultValue));
    }

    template<typename T>
    bool Set(std::string_view key, const T &value) {
        spdlog::info("{} Setting {} to {}", LOG_TAG, key, value);
        m_Document[key] = value;
        SaveToNVS();
        return true;
    }

    bool Set(std::string_view key, std::string_view value) {
        spdlog::info("{} Setting {} to {}", LOG_TAG, key, value);
        m_Document[key] = value;
        SaveToNVS();
        return true;
    }

    bool Set(std::string_view key, const char* value) {
        return Set(key, std::string_view(value));
    }

    bool Set(std::string_view key, const std::string &value) {
        return Set(key, std::string_view(value));
    }

    bool Set(std::string_view key, std::string &&value) {
        return Set(key, std::string_view(value));
    }

    template<typename T>
    T GetNested(std::string_view path, const T &defaultValue = T()) const {
        JsonVariantConst current = m_Document;
        std::string pathStr(path);
        size_t pos = 0;
        size_t delim;

        while ((delim = pathStr.find('.', pos)) != std::string::npos) {
            std::string_view token(pathStr.data() + pos, delim - pos);
            current = current[std::string(token).c_str()];
            if (current.isNull()) {
                return defaultValue;
            }
            pos = delim + 1;
        }

        std::string_view lastToken(pathStr.data() + pos);
        current = current[std::string(lastToken).c_str()];

        if (current.isNull() || !current.is<T>()) {
            return defaultValue;
        }
        return current.as<T>();
    }

    std::string GetNested(std::string_view path, std::string_view defaultValue = "") const {
        JsonVariantConst current = m_Document;
        std::string pathStr(path);
        size_t pos = 0;
        size_t delim;

        while ((delim = pathStr.find('.', pos)) != std::string::npos) {
            std::string_view token(pathStr.data() + pos, delim - pos);
            current = current[std::string(token).c_str()];
            if (current.isNull()) {
                return std::string(defaultValue);
            }
            pos = delim + 1;
        }

        std::string_view lastToken(pathStr.data() + pos);
        current = current[std::string(lastToken).c_str()];

        if (current.isNull() || !current.is<const char*>()) {
            return std::string(defaultValue);
        }
        return current.as<const char*>();
    }

    std::string GetNested(std::string_view path, const char* defaultValue = "") const {
        return GetNested(path, std::string_view(defaultValue));
    }

    std::string GetNested(std::string_view path, const std::string &defaultValue) const {
        return GetNested(path, std::string_view(defaultValue));
    }

    template<typename T>
    bool SetNested(std::string_view path, const T &value) {
        spdlog::info("{} Setting {} to {}", LOG_TAG, path, value);
        std::vector<std::string> parts;
        std::string pathStr(path);
        size_t start = 0, end;

        while ((end = pathStr.find('.', start)) != std::string::npos) {
            parts.push_back(pathStr.substr(start, end - start));
            start = end + 1;
        }
        parts.push_back(pathStr.substr(start));

        JsonObject current = m_Document.as<JsonObject>();
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current[parts[i]].is<JsonObject>()) {
                current[parts[i]] = JsonObject();
            }
            current = current[parts[i]];
        }
        current[parts.back()] = value;

        SaveToNVS();
        return true;
    }

    bool SetNested(std::string_view path, std::string_view value) {
        spdlog::info("{} Setting {} to {}", LOG_TAG, path, value);
        std::vector<std::string> parts;
        std::string pathStr(path);
        size_t start = 0, end;

        while ((end = pathStr.find('.', start)) != std::string::npos) {
            parts.push_back(pathStr.substr(start, end - start));
            start = end + 1;
        }
        parts.push_back(pathStr.substr(start));

        JsonObject current = m_Document.as<JsonObject>();
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current[parts[i]].is<JsonObject>()) {
                current[parts[i]] = JsonObject();
            }
            current = current[parts[i]];
        }
        current[parts.back()] = value;
        SaveToNVS();
        return true;
    }

    bool SetNested(std::string_view path, const char* value) {
        return SetNested(path, std::string_view(value));
    }

    bool SetNested(std::string_view path, const std::string &value) {
        return SetNested(path, std::string_view(value));
    }

    bool SetNested(std::string_view path, std::string &&value) {
        return SetNested(path, std::string_view(value));
    }
private:
    static constexpr const char* LOG_TAG = "[Config] -";
    std::string m_ConfigName;
    std::string m_NVSNamespace;
    JsonDocument m_Document;
};

#endif //LPRD_MCU_CONFIGMANAGER_H
