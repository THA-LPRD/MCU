#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace Config
{
#ifdef MCU_ESP32
    // ESP32 Devmodule
    enum Pin : uint8_t {
        BTN1 = 2,
        VCC = 43,
    };
#endif

#ifdef MCU_ESP32S3
    enum Pin : uint8_t {
        BTN1 = 2,
        VCC = 43,
    };
#endif

    enum class Key {
        OperatingMode,
        WiFiSSID,
        WiFiPassword,
        DisplayDriver,
        LogLevel,
        HTTPUsername,
        HTTPPassword,
        HTTPS,
        ServerURL,
        MAX
    };

    namespace
    {
        struct Item {
            Key key;
            const char* name;
            const char* default_value;
            std::function<bool(std::string_view)> validator = [](std::string_view) { return true; };
        };

        const std::unordered_map<Key, Item> m_Items = {
                {
                        Key::OperatingMode, {
                                                    Key::OperatingMode,
                                                    "OperatingMode",
                                                    "Default",
                                                    [](std::string_view value) {
                                                        return value == "Standalone" ||
                                                               value == "Network" ||
                                                               value == "Server" ||
                                                               value == "Default";
                                                    }}},
                {
                        Key::WiFiSSID,      {
                                                    Key::WiFiSSID,
                                                    "WiFiSSID",
                                                    "THA-LPRD-",
                                                    [](std::string_view value) {
                                                        return value.length() > 0;
                                                    }}},
                {
                        Key::WiFiPassword,  {
                                                    Key::WiFiPassword,
                                                    "WiFiPassword",
                                                    "password",
                                                    [](std::string_view value) {
                                                        return value.length() >= 8;
                                                    }}},
                {
                        Key::DisplayDriver, {
                                                    Key::DisplayDriver,
                                                    "DisplayDriver",
                                                    "WS_7IN3G",
                                                    [](std::string_view value) {
                                                        return value == "WS_7IN3G" || value == "WS_9IN7";
                                                    }}},
                {
                        Key::ServerURL, {
                        Key::ServerURL, "ServerURL", "", [](std::string_view value) {
                            return value.length() >= 0;
                        }}},
                {
                        Key::LogLevel,      {
                                                    Key::LogLevel,
                                                    "LogLevel",
                                                    "Info",
                                                    [](std::string_view value) {
                                                        return value == "Trace" || value == "Debug" ||
                                                               value == "Info" || value == "Warn" ||
                                                               value == "Error" || value == "Fatal";
                                                    }}},
                {
                        Key::HTTPUsername,  {
                                                    Key::HTTPUsername,
                                                    "HTTPUsername",
                                                    "admin",
                                                    [](std::string_view value) {
                                                        return value.length() > 0;
                                                    }}},
                {
                        Key::HTTPPassword,  {
                                                    Key::HTTPPassword,
                                                    "HTTPPassword",
                                                    "admin",
                                                    [](std::string_view value) {
                                                        return value.length() > 0;
                                                    }}},
                {
                        Key::HTTPS,         {
                                                    Key::HTTPS,
                                                    "HTTPS",
                                                    "false",
                                                    [](std::string_view value) {
                                                        return value == "true" || value == "false";
                                                    }}}
        };
        const std::unordered_map<std::string, Key> m_ReverseItems = []() {
            std::unordered_map<std::string, Key> reverse;
            for (const auto& item : m_Items) {
                reverse[item.second.name] = item.first;
            }
            return reverse;
            }();
    } // namespace

    std::string GetDefault(Key key);
    void LoadDefault();
    void Load();
    void Save();
    bool Set(Key key, std::string_view value);
    std::string Get(Key key);
} // namespace Config

#endif /*CONFIG_H_*/
