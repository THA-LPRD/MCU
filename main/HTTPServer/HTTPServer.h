#ifndef LPRD_MCU_HTTPSERVER_H
#define LPRD_MCU_HTTPSERVER_H

#include <spdlog/spdlog.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <string_view>
#include "../ConfigManager.h"
#include <charconv>
#include <fstream>
#include "../LoggingUtils.h"
#include "ServerVariable.h"
#include "magic_enum.hpp"
#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>
#include <PsychicRequest.h>
#include <LittleFS.h>
#include <Arduino.h>

class HTTPServer {
public:
    using Handler_t = std::function<esp_err_t(PsychicRequest*)>;

    explicit HTTPServer(std::string_view apiEndpoint = "/api");
    ~HTTPServer();
    bool Init();
    void SetFilesToServe(const std::map<std::string, std::string> &files);
    void AddEndpoint(std::string_view endpointPath, http_method method, const Handler_t &handlerFunction);
    void CreateVariable(const std::shared_ptr<std::string> &storage,
                        const std::function<bool(std::string_view)> &validator,
                        std::string_view paramName,
                        bool Get = true, bool Set = true);
    void CreateVariable(const std::function<std::string()> &getter,
                        const std::function<bool(std::string_view)> &setter,
                        std::string_view paramName,
                        bool Get = true, bool Set = true);
    void AddSetVarEndpoint(std::string_view endpointPath, const std::shared_ptr<ServerVariable> &variable);
    void AddGetVarEndpoint(std::string_view endpointPath, const std::shared_ptr<ServerVariable> &variable);
    void AddUploadEndpoint(std::string_view endpoint,
                           const std::function<std::string(std::string_view filename)> &getTargetPath,
                           const std::function<void(std::string_view)> &postUpload);
    inline const ConfigManager* GetConfig() { return &m_Config; }
private:
    bool InitHTTPS();
    bool InitHTTP();
private:
    static constexpr const char* LOG_TAG = "[HTTP Server] -";
    PsychicHttpServer m_HTTPServer;
    PsychicHttpsServer m_HTTPSServer;
    PsychicHttpServer* m_MainServer = nullptr;
    bool m_HTTPS = false;
    std::string m_APIEndpoint;
    std::string m_APIEndpointSet;
    std::string m_APIEndpointGet;
    std::vector<std::shared_ptr<ServerVariable>> m_Variables;
    ConfigManager m_Config = ConfigManager("httpserver");
};

#endif //LPRD_MCU_HTTPSERVER_H
