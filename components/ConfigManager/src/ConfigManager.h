#ifndef LPRD_MCU_CONFIGMANAGER_H
#define LPRD_MCU_CONFIGMANAGER_H

#include <spdlog/spdlog.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <magic_enum.hpp>
#include <ArduinoJson.h>
#include <charconv>

template<typename KeyEnum>
class ConfigManager {
public:
    using ValidatorFunc = std::function<bool(std::string_view)>;

    ConfigManager(std::shared_ptr<spdlog::logger> logger,
                  const std::unordered_map<KeyEnum, std::string> &defaultConfig,
                  const std::unordered_map<KeyEnum, ValidatorFunc> &validators = {})
            : m_Logger(logger),
            m_DefaultConfig(defaultConfig),
            m_Validators(validators) {
        LoadDefaults();
    }

    void LoadDefaults() {
        for (const auto &item: m_DefaultConfig) {
            m_Config[item.first] = item.second;
        }
        m_Logger->info("Loaded default configuration");
    }

    void LoadFromFile(const std::string &filename) {
        m_Logger->info("Loading config from file: {}", filename);
        // TODO: Implement loading from file
    }

    void SaveToFile(const std::string &filename) {
        m_Logger->info("Saving config to file: {}", filename);
        // TODO: Implement saving to file
    }

    bool Set(KeyEnum key, std::string_view value) {
        if (IsValid(key, value)) {
            m_Config[key] = std::string(value);
            m_Logger->info("Set key {} to {}", magic_enum::enum_name(key), value);
            return true;
        }
        m_Logger->debug("Invalid value for key {}", magic_enum::enum_name(key));
        return false;
    }

    bool Set(std::string_view key, std::string_view value) {
        for (const auto &[k, v]: m_Config) {
            if (magic_enum::enum_name(k) == key) {
                if (IsValid(k, value)) {
                    m_Config[k] = std::string(value);
                    m_Logger->info("Set key {} to {}", key, value);
                    return true;
                }
                m_Logger->debug("Invalid value for key {}", key);
                return false;
            }
        }
        m_Logger->debug("Key not found: {}", key);
        return false;
    }

    std::string GetString(KeyEnum key) const {
        auto it = m_Config.find(key);
        if (it != m_Config.end()) {
            return it->second;
        }
        m_Logger->debug("Key not found: {}", magic_enum::enum_name(key));
        return "";
    }

    std::string GetString(std::string_view key) const {
        for (const auto &[k, v]: m_Config) {
            if (magic_enum::enum_name(k) == key) {
                return v;
            }
        }
        m_Logger->debug("Key not found: {}", key);
        return "";
    }

    int GetInt(KeyEnum key) const {
        auto it = m_Config.find(key);
        if (it != m_Config.end()) {
            int value;
            auto [ptr, ec] = std::from_chars(it->second.data(), it->second.data() + it->second.size(), value);
            if (ec == std::errc{}) {
                return value;
            }
        }
        m_Logger->debug("Key not found or does not contain a valid integer: {}", magic_enum::enum_name(key));
        return 0;
    }

    int GetInt(std::string_view key) const {
        for (const auto &[k, v]: m_Config) {
            if (magic_enum::enum_name(k) == key) {
                int value;
                auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), value);
                if (ec == std::errc{}) {
                    return value;
                }
            }
        }
        m_Logger->debug("Key not found or does not contain a valid integer: {}", key);
        return 0;
    }

    void SetValidator(KeyEnum key, ValidatorFunc validator) {
        m_Validators[key] = validator;
    }

    void SetValidator(std::string_view key, ValidatorFunc validator) {
        for (const auto &[k, v]: m_Config) {
            if (magic_enum::enum_name(k) == key) {
                m_Validators[k] = validator;
                return;
            }
        }
        m_Logger->debug("Key not found: {}", key);
    }
private:
    bool IsValid(KeyEnum key, std::string_view value) const {
        auto it = m_Validators.find(key);
        if (it != m_Validators.end()) {
            return it->second(value);
        }
        return true; // If no validator, assume valid
    }
private:
    std::shared_ptr<spdlog::logger> m_Logger;
    std::unordered_map<KeyEnum, std::string> m_Config;
    std::unordered_map<KeyEnum, std::string> m_DefaultConfig;
    std::unordered_map<KeyEnum, ValidatorFunc> m_Validators;
};


#endif //LPRD_MCU_CONFIGMANAGER_H
