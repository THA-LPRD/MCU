#include "AppHost.h"
#include "SD.h"
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

        // File dir = LittleFS.open(currentPath.c_str());
        File dir = SD.open(currentPath.c_str());
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
            }
            else {
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

    // m_Server.AddUploadEndpoint(
    //         "/api/v2/UploadSlot1Img",
    //         [](std::string_view filename) {
    //             spdlog::info("{} Uploaded Image on Slot 1", LOG_TAG);
    //             return "/" + std::string("slot1.png");
    //         }
    // );

    // m_Server.AddUploadEndpoint(
    //         "/api/v2/UploadSlot2Img",
    //         [](std::string_view filename) {
    //             spdlog::info("{} Uploaded Image on Slot 2", LOG_TAG);
    //             return "/" + std::string("slot2.png");
    //         }
    // );

    // m_Server.AddUploadEndpoint(
    //         "/api/v2/UploadSlot3Img",
    //         [](std::string_view filename) {
    //             spdlog::info("{} Uploaded Image on Slot 3", LOG_TAG);
    //             return "/" + std::string("slot3.png");
    //         }
    // );

    // m_Server.AddUploadEndpoint(
    //         "/api/v2/UploadSlot4Img",
    //         [](std::string_view filename) {
    //             spdlog::info("{} Uploaded Image on Slot 4", LOG_TAG);
    //             return "/" + std::string("slot4.png");
    //         }
    // );

    // m_Server.CreateVariable(
    //         [this]() { return m_ConfigApplication.Get("CurrtenSlot", 0);},
    //         [this](std::string_view value) {
    //             std::string str(value);
    //             int slot = str.strtoi();
    //             m_ConfigApplication.Set("CurrtenSlot", slot);
    //             // TODO 
    //             // Slot in str
    //             spdlog::info("{} Processing image", LOG_TAG);
    //             int handle = m_Display->CreateImage("/img.png");
    //             spdlog::debug("{} Got handle: %d", LOG_TAG, handle);
    //             this->m_Display->BeginFrame();
    //             this->m_Display->DrawImage(handle, 0, 0);
    //             this->m_Display->SwapBuffers();
    //             this->m_Display->EndFrame();
    //             this->m_Display->DeleteImage(handle);
    //             spdlog::debug("{} Ready for next image", LOG_TAG);
    //             this->m_SleepTime = UINT64_MAX;
    //             this->m_Running = false;
               
    //             return true;
    //         },
    //         "CurrtenSlot"
    // );

    m_Server.AddEndpointText(
            "/api/v2/Restart",
            http_method::HTTP_POST,
            [this](PsychicRequest* request) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                this->m_SleepTime = 0;
                this->m_Running = false;
                return request->reply(200);
            }
    );

    m_Server.AddEndpointText(
            "/api/v2/DeviceInfo",
            http_method::HTTP_GET,
            [this](PsychicRequest* request) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                std::string response;
                JsonDocument json;

                json["DeviceID"] = m_DeviceID;
                json["OperatingMode"] = m_ConfigApplication.Get<std::string_view>("OperatingMode");
                json["IP"] = std::string(ip4addr_ntoa(&m_IP));
                json["LogLevel"] = m_ConfigApplication.Get<std::string_view>("LogLevel");

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
    m_Server.AddEndpointText(
            "/api/v2/DeviceConfig",
            http_method::HTTP_GET,
            [this](PsychicRequest* request) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                auto value = m_ConfigApplication.Get<std::string>("OperatingMode");
                esp_err_t ret = request->reply(200, "plain/text", value.c_str());
                if (ret != ESP_OK) {
                    spdlog::error("{} Get failed: could not reply: {}", LOG_TAG, esp_err_to_name(ret));
                }
                else {
                    spdlog::debug("{} Get success: {} -> {}", LOG_TAG, "OperatingMode", value);
                }

                return ret;
            }
    );

    m_Server.AddEndpointJson(
            "/api/v2/DeviceConfig",
            http_method::HTTP_POST,
            [this](PsychicRequest* request, JsonVariant &json) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                std::string response;

                auto modestr = json["Mode"].as<std::string>();
                auto wifiSSIDstr = json["WiFiSSID"].as<std::string>();
                auto wifiPassstr = json["WiFiPassword"].as<std::string>();
                auto serverURLstr = json["ServerURL"].as<std::string>();

                if (modestr.empty()) {
                    spdlog::error("{} Set failed: Missing Mode parameter", LOG_TAG);
                    return request->reply(400, "text/plain", "Missing Mode parameter");
                }
                if (modestr != "Standalone" && modestr != "Network" && modestr != "Server") {
                    spdlog::error("{} Set failed: Invalid Mode parameter", LOG_TAG);
                    return request->reply(400, "text/plain", "Invalid Mode parameter");
                }
                if (wifiSSIDstr.empty()) {
                    spdlog::error("{} Set failed: Missing WiFiSSID parameter", LOG_TAG);
                    return request->reply(400, "text/plain", "Missing WiFiSSID parameter");
                }
                if (wifiPassstr.empty()) {
                    spdlog::error("{} Set failed: Missing WiFiPass parameter", LOG_TAG);
                    return request->reply(400, "text/plain", "Missing WiFiPass parameter");
                }
                if (wifiSSIDstr.length() > 32) {
                    spdlog::error("{} Set failed: WiFiSSID parameter too long", LOG_TAG);
                    return request->reply(400, "text/plain", "WiFi SSID parameter too long");
                }
                if (wifiPassstr.length() < 8 || wifiPassstr.length() > 63) {
                    spdlog::error("{} Set failed: WiFiPass parameter invalid length", LOG_TAG);
                    return request->reply(400,
                                          "text/plain",
                                          "WiFi Password parameter cannot be less than 8 or more than 63 characters");
                }
                if (modestr == "Server") {
                    if (serverURLstr.empty()) {
                        spdlog::error("{} Set failed: Missing ServerURL parameter", LOG_TAG);
                        return request->reply(400, "text/plain", "Missing ServerURL parameter");
                    }
                    if (serverURLstr.length() > 255) {
                        spdlog::error("{} Set failed: ServerURL parameter too long", LOG_TAG);
                        return request->reply(400, "text/plain", "Server URL parameter too long");
                    }
                    m_ConfigApplication.SetNested("AppServer.WiFi.SSID", wifiSSIDstr);
                    m_ConfigApplication.SetNested("AppServer.WiFi.Password", wifiPassstr);
                    m_ConfigApplication.SetNested("AppServer.ServerURL", serverURLstr);
                }
                if (modestr == "Standalone") {
                    m_ConfigApplication.SetNested("AppStandalone.WiFi.SSID", wifiSSIDstr);
                    m_ConfigApplication.SetNested("AppStandalone.WiFi.Password", wifiPassstr);
                }
                else if (modestr == "Network") {
                    m_ConfigApplication.SetNested("AppNetwork.WiFi.SSID", wifiSSIDstr);
                    m_ConfigApplication.SetNested("AppNetwork.WiFi.Password", wifiPassstr);
                }
                m_ConfigApplication.Set("OperatingMode", modestr);

                return request->reply(200);
            }
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigPeripherals.GetNested<std::string>("Display.Driver"); },
            [this](std::string_view value) {
                if (!m_ConfigPeripherals.SetNested("Display.Driver", value)) {
                    return false;
                }
                this->m_Display->Start(magic_enum::enum_cast<EPDL::Display>(value).value());
                return true;
            },
            "DisplayDriver"
    );

    m_Server.CreateVariable(
            [this]() { return m_ConfigApplication.Get<std::string>("LogLevel"); },
            [this](std::string_view value) {
                m_ConfigApplication.Set("LogLevel", value);
                spdlog::set_level(spdlog::level::from_str(value.data()));
                return true;
            },
            "LogLevel"
    );
}

void AppHost::InitServerHTTP() {
    m_Server.CreateVariable(
            [this]() { return std::to_string(this->m_Server.GetConfig()->Get("Port", 80)); },
            [this](std::string_view value) {
                int port = std::stoi(std::string(value));
                if (port < 1 || port > 65535) {
                    return false;
                }
                return this->m_Server.GetConfig()->Set("Port", port);
            },
            "Http"
    );

    m_Server.AddEndpointText(
            "/api/v2/Https",
            http_method::HTTP_GET,
            [this](PsychicRequest* request) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                std::string response;
                JsonDocument json;

                json["Enabled"] = this->m_Server.GetConfig()->GetNested<bool>("SSL.Enabled", false);
                json["Port"] = this->m_Server.GetConfig()->GetNested<int>("SSL.Port", 443);
                json["HasCert"] = SD.exists("/https.crt");
                json["HasKey"] = SD.exists("/https.key");
                // json["HasCert"] = LittleFS.exists("/https.crt");
                // json["HasKey"] = LittleFS.exists("/https.key");

                serializeJson(json, response);

                return request->reply(200, "application/json", response.c_str());
            }
    );

    m_Server.AddEndpointJson(
            "/api/v2/Https",
            http_method::HTTP_POST,
            [this](PsychicRequest* request, JsonVariant &json) -> esp_err_t {
                spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                std::string response;

                if (!json["Enabled"].as<bool>()) {
                    this->m_Server.GetConfig()->SetNested("SSL.Enabled", false);
                    SD.remove("/https.crt");
                    SD.remove("/https.key");
                    // LittleFS.remove("/https.crt");
                    // LittleFS.remove("/https.key");
                    return request->reply(200);
                }

                auto writeFile = [](std::string_view filename, const uint8_t* data, size_t len) -> bool {
                    File file = SD.open(filename.data(), "w");
                    // File file = LittleFS.open(filename.data(), "w");
                    if (!file) {
                        spdlog::error("{} Failed to open file for writing: {}", LOG_TAG, filename);
                        return false;
                    }

                    if (file.write(data, len) != len) {
                        spdlog::error("{} Failed to write to file: {}", LOG_TAG, filename);
                        return false;
                    }

                    file.close();
                    spdlog::info("{} Wrote {} bytes to {}", LOG_TAG, len, filename);
                    return true;
                };

                if (!this->m_Server.GetConfig()->GetNested<bool>("SSL.Enabled", false)) { // Activate
                    if (!json["Cert"].is<std::string>() || !json["Key"].is<std::string>()) {
                        return request->reply(400,
                                              "text/plain",
                                              "Certificate and key files are required when enabling HTTPS");
                    }

                    if (!writeFile("/https.crt",
                                   reinterpret_cast<const uint8_t*>(json["Cert"].as<std::string>().c_str()),
                                   json["Cert"].as<std::string>().length())) {
                        return request->reply(500);
                    }

                    if (!writeFile("/https.key",
                                   reinterpret_cast<const uint8_t*>(json["Key"].as<std::string>().c_str()),
                                   json["Key"].as<std::string>().length())) {
                        return request->reply(500);
                    }

                    if (json["Port"].is<int>()) {
                        this->m_Server.GetConfig()->SetNested("SSL.Port", json["Port"].as<int>());
                    }

                    this->m_Server.GetConfig()->SetNested("SSL.Enabled", true);

                    return request->reply(200);
                }
                else { // Update
                    if (json["Cert"].is<std::string>()) {
                        if (!writeFile("/https.crt",
                                       reinterpret_cast<const uint8_t*>(json["Cert"].as<std::string>().c_str()),
                                       json["Cert"].as<std::string>().length())) {
                            return request->reply(500);
                        }
                    }

                    if (json["Key"].is<std::string>()) {
                        if (!writeFile("/https.key",
                                       reinterpret_cast<const uint8_t*>(json["Key"].as<std::string>().c_str()),
                                       json["Key"].as<std::string>().length())) {
                            return request->reply(500);
                        }
                    }

                    if (json["Port"].is<int>()) {
                        this->m_Server.GetConfig()->SetNested("SSL.Port", std::to_string(json["Port"].as<int>()));
                    }

                    return request->reply(200);
                }
            }
    );

    m_Server.CreateVariable(
            [this]() {
                bool enabled = this->m_Server.GetConfig()->GetNested<bool>("Auth.Enabled");
                return enabled ? "true" : "false";
            },
            [this](std::string_view value) {
                if (value != "true" && value != "false") {
                    return false;
                }
                bool enabled = value == "true";
                return this->m_Server.GetConfig()->SetNested("Auth.Enabled",
                                                             enabled);
            },
            "HttpAuth"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested<std::string>("Auth.Username"); },
            [this](std::string_view value) {
                return this->m_Server.GetConfig()->SetNested("Auth.Username",
                                                             value);
            },
            "HttpAuthUser"
    );

    m_Server.CreateVariable(
            [this]() { return this->m_Server.GetConfig()->GetNested<std::string>("Auth.Password"); },
            [this](std::string_view value) {
                return this->m_Server.GetConfig()->SetNested("Auth.Password",
                                                             value);
            },
            "HttpAuthPass"
    );
}
