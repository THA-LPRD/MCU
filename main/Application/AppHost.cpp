#include "AppHost.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include "esp_system.h"
#include <map>
#include <Drivers/Time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <queue>

AppHost::~AppHost() {
    spdlog::info("{} Destroyed AppHost", LOG_TAG);
}

static std::map<std::string, std::string> MapFilesRecursively(std::string_view basePath) {
    std::map<std::string, std::string> fileMap;
    std::queue<std::string> dirQueue;

    std::string baseStr(basePath);
    if (baseStr.back() != '/') baseStr += '/';
    dirQueue.push(baseStr);

    // Add root directory
    size_t wwwPos = baseStr.find("/www/");
    if (wwwPos != std::string::npos) {
        std::string rootUri = "/";
        fileMap[rootUri] = baseStr;
    }

    while (!dirQueue.empty()) {
        std::string currentPath = dirQueue.front();
        dirQueue.pop();

        File dir = LittleFS.open(currentPath.c_str());
        if (!dir || !dir.isDirectory()) {
            continue;
        }

        File file = dir.openNextFile();
        while (file) {
            std::string fullPath = file.path();
            std::string fileName = file.name();

            // Find the position after /www/ in the full path
            wwwPos = fullPath.find("/www/");
            if (wwwPos == std::string::npos) {
                file = dir.openNextFile();
                continue;
            }

            std::string relativePath = fullPath.substr(wwwPos + 4); // +4 to keep leading slash

            if (file.isDirectory()) {
                // Add directory with trailing slash to queue and map
                std::string dirPath = fullPath + "/";
                std::string dirUri = relativePath + "/";
                dirQueue.push(dirPath);
                fileMap[dirUri] = dirPath;
            } else {
                // Handle files
                std::string uri;
                if (fileName == "index.html") {
                    // For index.html, use the directory path
                    uri = relativePath.substr(0, relativePath.length() - 10); // Remove "index.html"
                    if (uri == "/www") uri = "/"; // Root index.html
                }
                else if (fileName.ends_with(".html")) {
                    // For other HTML files, remove the .html extension
                    uri = relativePath.substr(0, relativePath.length() - 5);
                }
                else {
                    // For all other files, keep the extension
                    uri = relativePath;
                }

                // Add the mapping
                fileMap[uri] = fullPath;
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    return fileMap;
}

bool AppHost::InitServer() {
    if (!m_Server.Init()) return false;
    std::map<std::string, std::string> filesToServe = MapFilesRecursively("/www");
    filesToServe["/"] = "/www/";

    m_Server.SetFilesToServe(filesToServe);

    InitServerCore();
    InitServerStandalone();
    InitServerNetwork();
    InitServerServer();
    InitServerHTTP();

    m_Server.AddUploadEndpoint(
            "/api/v2/UploadImg",
            [](std::string_view filename) {
                return "/" + std::string("img.png");
            },
            [this](std::string_view filename) {
                spdlog::info("{} Processing image", LOG_TAG);
                int handle = m_Display->CreateImage("/img.png");
                spdlog::debug("{} Got handle: %d", LOG_TAG, handle);
                this->m_Display->BeginFrame();
                this->m_Display->DrawImage(handle, 0, 0);
                this->m_Display->SwapBuffers();
                this->m_Display->EndFrame();
                this->m_Display->DeleteImage(handle);
                spdlog::debug("{} Ready for next image", LOG_TAG);
                this->m_SleepTime = UINT64_MAX;
                this->m_Running = false;
            }
    );

    m_Server.AddUploadEndpoint(
            "/api/v2/UploadHttpsCert",
            [](std::string_view filename) {
                return "/" + std::string("https.crt");
            },
            [](std::string_view filename) { return; }
    );

    m_Server.AddUploadEndpoint(
            "/api/v2/UploadHttpsKey",
            [](std::string_view filename) {
                return "/" + std::string("https.key");
            },
            [](std::string_view filename) { return; }
    );

    m_Server.AddEndpoint(
            "/api/v2/Restart",
            http_method::HTTP_POST,
            [this](PsychicRequest* request) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                this->m_SleepTime = 0;
                this->m_Running = false;
                return request->reply(200, "text/plain", "OK");
            }
    );

    m_Server.AddEndpoint(
            "/api/v2/DeviceInfo",
            http_method::HTTP_GET,
            [this](PsychicRequest* request) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                std::string response;
                JsonDocument json;

                json["DeviceID"] = m_DeviceID;
                json["OperatingMode"] = m_ConfigApplication.Get("OperatingMode");
                json["IP"] = std::string(ip4addr_ntoa(&m_IP));
                json["LogLevel"] = m_ConfigApplication.Get("LogLevel");

                DriverInfo driverInfo = m_Display->GetDriverInfo();
                JsonObject display = json["Display"].to<JsonObject>();
                display["Width"] = driverInfo.Width;
                display["Height"] = driverInfo.Height;
                display["ColorDepth"] = driverInfo.ColorDepth;
                display["PartialRefresh"] = driverInfo.PartialRefresh;
                display["DriverName"] = driverInfo.DriverName;

                serializeJson(json, response);

                return request->reply(200, "application/json", response.c_str());
            }
    );

    m_Server.CreateVariable(
            []() {
                std::string timestr;
                time_t t = Time::Get();
                timestr = std::to_string(t);
                return std::string(timestr);
            },
            [](std::string_view value) {
                std::string str(value);
                time_t t = 0;

                if (str.empty() || !std::all_of(str.begin(), str.end(), ::isdigit)) {
                    return false;
                }

                t = std::strtol(str.c_str(), nullptr, 10);
                if (t == 0 || errno == ERANGE) {
                    return false;
                }

                Time::Set(t);
                return true;

            },
            "Time"
    );

    return true;
}

void AppHost::InitServerCore() {
    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.Get("OperatingMode"); },
            [this](std::string_view value) {
                return m_ConfigApplication.Set("OperatingMode", value);
            },
            "OpMode"
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.Get("DisplayDriver"); },
            [this](std::string_view value) {
                return m_ConfigApplication.Set("DisplayDriver", value);
            },
            "displayModule"
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.Get("LogLevel"); },
            [this](std::string_view value) {
                m_ConfigApplication.Set("LogLevel", value);
                spdlog::set_level(spdlog::level::from_str(value.data()));
                return true;
            },
            "LogLevel"
    );
}

void AppHost::InitServerStandalone() {
    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppStandalone.WiFi.SSID"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppStandalone.WiFi.SSID", value);
            },
            "StandaloneSSID",
            false, true
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppStandalone.WiFi.Password"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppStandalone.WiFi.Password", value);
            },
            "StandalonePassword",
            false, true
    );
}

void AppHost::InitServerNetwork() {
    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppNetwork.WiFi.SSID"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppNetwork.WiFi.SSID", value);
            },
            "NetworkSSID",
            false, true
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppNetwork.WiFi.Password"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppNetwork.WiFi.Password", value);
            },
            "NetworkPassword",
            false, true
    );
}

void AppHost::InitServerServer() {
    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppServer.WiFi.SSID"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppServer.WiFi.SSID", value);
            },
            "ServerSSID",
            false, true
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppServer.WiFi.Password"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppServer.WiFi.Password", value);
            },
            "ServerPassword",
            false, true
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.GetNested("AppServer.ServerURL"); },
            [this](std::string_view value) {
                return m_ConfigApplication.SetNested("AppServer.ServerURL", value);
            },
            "ServerURL"
    );
}

void AppHost::InitServerHTTP() {
    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested("HTTP.Port"); },
            [this](std::string_view value) { return this->m_Server.GetConfig()->SetNested("HTTP.Port", value); },
            "HttpPort"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested("SSL.Port"); },
            [this](std::string_view value) { return this->m_Server.GetConfig()->SetNested("SSL.Port", value); },
            "HttpsPort"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested("SSL.Enabled"); },
            [this](std::string_view value) { return this->m_Server.GetConfig()->SetNested("SSL.Enabled", value); },
            "Https"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested("HTTP.Auth.Enabled"); },
            [this](std::string_view value) {
                return this->m_Server.GetConfig()->SetNested("HTTP.Auth.Enabled",
                                                             value);
            },
            "HttpAuth"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested("HTTP.Auth.Username"); },
            [this](std::string_view value) {
                return this->m_Server.GetConfig()->SetNested("HTTP.Auth.Username",
                                                             value);
            },
            "HttpAuthUser"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested("HTTP.Auth.Password"); },
            [this](std::string_view value) {
                return this->m_Server.GetConfig()->SetNested("HTTP.Auth.Password",
                                                             value);
            },
            "HttpAuthPass"
    );
}
