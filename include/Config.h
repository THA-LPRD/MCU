#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace Config
{
    namespace Pin
    {
        constexpr uint8_t RST = 2;
    } // namespace Pin

    enum class Key {
        OperatingMode,
        WiFiSSID,
        WiFiPassword,
        DisplayDriver,
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
                        Key::OperatingMode, "OperatingMode", "Standalone", [](std::string_view value) {
                            return value == "Standalone" ||
                                   value == "Network" ||
                                   value == "Server" ||
                                   value == "Default";
                        }}},
                {
                        Key::WiFiSSID, {
                        Key::WiFiSSID, "WiFiSSID", "THA-LPRD-001", [](std::string_view value) {
                            return value.length() > 0;
                        }}},
                {
                        Key::WiFiPassword, {
                        Key::WiFiPassword, "WiFiPassword", "password", [](std::string_view value) {
                            return value.length() >= 8;
                        }}},
                {
                        Key::DisplayDriver, {
                        Key::DisplayDriver, "DisplayDriver", "WS_7IN3G", [](std::string_view value) {
                            return value == "WS_7IN3G";
                        }}}
                };
                const std::unordered_map<std::string, Key> m_ReverseItems =[]() {
                    std::unordered_map<std::string, Key> reverse;
                    for (const auto &item: m_Items) {
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
