#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>

#define DEFINE_CONFIG_KEY(name, type) \
    void Set##name(const type& value); \
    const type& Get##name();

#define DEFAULT_WIFI_AP_SSID  "THA-LPRD-001"
#define DEFAULT_WIFI_PASSWORD  "password"

namespace Config
{
    void LoadDefaultConfig();
    void LoadConfig();
    void SaveConfig();
    DEFINE_CONFIG_KEY(OperatingMode, std::string);
    DEFINE_CONFIG_KEY(WiFiSSID, std::string);
    DEFINE_CONFIG_KEY(WiFiPassword, std::string);
} // namespace Config
#undef DEFINE_CONFIG_KEY

#endif /*CONFIG_H_*/
