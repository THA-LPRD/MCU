#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include "Log.h"

class Config {
public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    inline static Config& GetInstance() {
        static Config m_Instance;
        return m_Instance;
    }
    static void LoadDefaultConfig();
    static void LoadConfig();
    static void SaveConfig();
private:
    Config() = default;
    ~Config();
private:
    std::string m_FileName = "/littlefs/config.json";

#define DEFINE_CONFIG_KEY(name, type) \
    private: \
    type m_##name; \
    public: \
        inline static void Set##name(const type& value) \
        { GetInstance().m_##name = value; } \
        inline static const type& Get##name() \
        { return GetInstance().m_##name; }

    DEFINE_CONFIG_KEY(OperatingMode, std::string);
    DEFINE_CONFIG_KEY(WiFiSSID, std::string);
    DEFINE_CONFIG_KEY(WiFiPassword, std::string);

#undef DEFINE_CONFIG_KEY
};

#endif /*CONFIG_H_*/
