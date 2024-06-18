#include <Arduino.h>
#include <fstream>
#include <LittleFS.h>
#include "Config.h"
#include "HTTPServer.h"
#include <Log.h>
#include <MCU.h>
#include <Filesystem.h>
#include <EPDL.h>

HTTPServer::~HTTPServer() {
    Log::Debug("[HTTPServer] Destroying HTTP server");
    for (auto handler: m_Handlers) {
        delete handler;
    }
    m_Handlers.clear();
    if (m_isHTTPS) m_HTPPSServer.stop();
    m_HTTPServer.stop();
}

bool HTTPServer::Init(bool https) {
    Log::Debug("[HTTPServer] Initializing server");
    m_isHTTPS = https;
    m_MainServer = m_isHTTPS ? (PsychicHttpServer*) &m_HTPPSServer : &m_HTTPServer;
    bool status = m_isHTTPS ? InitHTTPS() : InitHTTP();
    if (!status) {
        Log::Error("[HTTPServer] Failed to initialize server");
        return false;
    }

    Log::Info("[HTTPServer] Server initialized");
    return true;
}

std::string ReadPemFile(std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        Log::Error("[HTTPServer] Failed to open file: %s", path.data());
        return "";
    }
    std::string pem((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return pem;
}

bool HTTPServer::InitHTTPS() {
    Log::Debug("[HTTPServer] Initializing HTTPS server");
    // Check if the certificate and key files exist
    if (!MCU::Filesystem::exists("/server.crt") || !MCU::Filesystem::exists("/server.key")) {
        Log::Error("[HTTPServer] Certificate or key file not found falling back to HTTP");
        Config::Set(Config::Key::HTTPS, "false");
        Config::Save();
        m_isHTTPS = false;
        m_MainServer = &m_HTTPServer;
        return InitHTTP();
    }
    m_HTPPSServer.config.max_uri_handlers = 50;
    m_HTPPSServer.ssl_config.httpd.max_uri_handlers = 50;
    m_HTPPSServer.ssl_config.httpd.stack_size = 64000;
    m_HTPPSServer.ssl_config.httpd.max_open_sockets = 8;
    std::string cert = ReadPemFile("/filesystem/server.crt");
    std::string key = ReadPemFile("/filesystem/server.key");
    esp_err_t err = m_HTPPSServer.listen(443, cert.c_str(), key.c_str());

    if (err != ESP_OK) {
        Log::Error("[HTTPServer] Failed to start HTTPS server: %s", esp_err_to_name(err));
        return false;
    }
    m_HTPPSServer.onNotFound([](PsychicRequest* request) {
        Log::Debug("Not found: %s", request->url().c_str());
        return request->reply(404, "text/plain", "Page not found");
    });
    m_HTTPServer.config.ctrl_port = 20420; // just a random port different from the default one
    err = m_HTTPServer.listen(80);
    if (err != ESP_OK) {
        Log::Error("[HTTPServer] Failed to start HTTP redirect server: %s", esp_err_to_name(err));
        m_HTTPServer.stop();
        return false;
    }
    m_HTTPServer.onNotFound([](PsychicRequest* request) {
        std::string url = "https://";
        url += request->host().c_str();
        url += request->url().c_str();
        return request->redirect(url.c_str());
    });

    Log::Info("[HTTPServer] HTTPS server initialized");
    return true;
}

bool HTTPServer::InitHTTP() {
    Log::Debug("[HTTPServer] Initializing HTTP server");
    m_HTTPServer.config.max_uri_handlers = 50;
    esp_err_t err = m_HTTPServer.listen(80);
    if (err != ESP_OK) {
        Log::Error("[HTTPServer] Failed to start server: %s", esp_err_to_name(err));
        return false;
    }
    m_HTTPServer.onNotFound([](PsychicRequest* request) {
        Log::Debug("Not found: %s", request->url().c_str());
        return request->reply(404, "text/plain", "Page not found");
    });

    Log::Info("[HTTPServer] HTTP server initialized");
    return true;
}

void HTTPServer::SetFilesToServe(const std::map<std::string, std::string> &files) {
    Log::Debug("[HTTPServer] Setting files to serve");
    for (const auto &file: files) {
        Log::Debug("[HTTPServer] Adding file %s to serve from %s", file.second.c_str(), file.first.c_str());
        auto handler = new PsychicStaticFileHandler(file.first.c_str(),
                                                    LittleFS,
                                                    file.second.c_str(),
                                                    nullptr);
        m_MainServer->addHandler(handler);
        m_Handlers.push_back(handler);
    }
    m_MainServer->on("/", HTTP_GET, [](PsychicRequest* request) {
        std::string url = "http://";
        url += request->host().c_str();
        url += "/index.html";
        return request->redirect(url.c_str());
    });
}

void HTTPServer::AddAPISetOpMode() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetOpMode");
    auto SetOpModeHandler = new PsychicWebHandler();
    SetOpModeHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received ConfigOpMode request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("opmode")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing Operating Mode");
        }

        PsychicWebParameter* pOpMode = request->getParam("opmode");
        if (!Config::Set(Config::Key::OperatingMode, pOpMode->value().c_str())) {
            Log::Error("Invalid Operating Mode");
            return request->reply(400, "text/plain", "Invalid Operating Mode");
        }

        Log::Debug("[HTTPServer] Operating Mode set to %s", Config::Get(Config::Key::OperatingMode).c_str());
        Config::Save();
        return request->reply(200, "text/plain", "Operating Mode set. Restart is needed to apply changes.");
    });
    m_MainServer->on("/api/v1/SetOpMode", HTTP_POST, SetOpModeHandler);
    m_Handlers.push_back(SetOpModeHandler);
}

void HTTPServer::AddAPISetWiFiCred() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetWiFiCred");
    auto SetWiFiCredHandler = new PsychicWebHandler();
    SetWiFiCredHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received SetWiFiCred request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("ssid") && !request->hasParam("password")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing SSID or password");
        }

        PsychicWebParameter* pSSID = request->getParam("ssid");
        PsychicWebParameter* pPassword = request->getParam("password");

        if (!Config::Set(Config::Key::WiFiSSID, pSSID->value().c_str())
            || !Config::Set(Config::Key::WiFiPassword, pPassword->value().c_str())) {
            Log::Error("[HTTPServer] Invalid WiFi credentials");
            return request->reply(400, "text/plain", "Invalid WiFi credentials");
        }

        Log::Debug("[HTTPServer] WiFi credentials set to %s, %s", pSSID->value().c_str(), pPassword->value().c_str());
        Config::Save();
        return request->reply(200, "text/plain", "WiFi credentials set. Restart is needed to apply changes.");
    });
    m_MainServer->on("/api/v1/SetWiFiCred", HTTP_POST, SetWiFiCredHandler);
    m_Handlers.push_back(SetWiFiCredHandler);
}

void HTTPServer::AddAPIUploadImg(const std::function<void(std::string_view filePath)> &onUpload) {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/UploadImg");
    if (!MCU::Filesystem::exists("/upload")) { MCU::Filesystem::mkdir("/upload"); }

    auto UploadImgHandler = new PsychicUploadHandler();
    UploadImgHandler->onUpload([onUpload](PsychicRequest* request, const String &filename, uint64_t index,
                                          uint8_t* data,
                                          size_t len,
                                          bool last) {
        bool* status;
        std::ofstream file;
        std::string path = "/upload/";
        path += filename.c_str();
        std::string pathNative = MCU::Filesystem::GetPath(path);
        bool isFirst = index == 0;

        if (isFirst) {
            Log::Info("[HTTPServer] Image upload started from client %s to %s",
                      request->client()->remoteIP().toString().c_str(),
                      pathNative.c_str());
            file.open(pathNative, std::ios::binary | std::ios::trunc);
            status = new bool(true);
            request->_tempObject = status;
        }
        else {
            status = static_cast<bool*>(request->_tempObject);
            if (!*status) { return ESP_FAIL; }
            file.open(pathNative, std::ios::binary | std::ios::app);
        }

        if (!file.is_open()) {
            Log::Error("[HTTPServer] Failed to open file");
            *status = false;
            return ESP_FAIL;
        }

        if (!file.write((char*) data, len)) {
            Log::Error("[HTTPServer] Write failed");
            *status = false;
            file.close();
            return ESP_FAIL;
        }

        file.close();

        Log::Debug("[HTTPServer] Wrote %d bytes", len);
        if (last) {
            Log::Debug("[HTTPServer] Calling callback: %s", pathNative.c_str());
            onUpload(path);
            Log::Debug("[HTTPServer] Image upload finished");
        }

        return ESP_OK;
    });

    // Called after upload has been handled
    UploadImgHandler->onRequest([](PsychicRequest* request) {
        bool status = true;
        if (request->_tempObject) { status = *static_cast<bool*>(request->_tempObject); }
        delete static_cast<bool*>(request->_tempObject);
        request->_tempObject = nullptr;

        if (!status) {
            Log::Error("[HTTPServer] Image upload failed");
            return request->reply(500, "text/plain", "Image upload failed");
        }

        Log::Info("[HTTPServer] Image upload successful");
        return request->reply(200, "text/plain", "Image successfully uploaded");
    });

    m_MainServer->on("/api/v1/UploadImg", HTTP_POST, UploadImgHandler);
    m_Handlers.push_back(UploadImgHandler);
}

void HTTPServer::AddAPIGetDisplayModule() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetDisplayModule");
    auto GetDisplayModuleHandler = new PsychicWebHandler();
    GetDisplayModuleHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received GetDisplayModule request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string displayModule = Config::Get(Config::Key::DisplayDriver);
        return request->reply(200, "text/plain", displayModule.c_str());
    });
    m_MainServer->on("/api/v1/GetDisplayModule", HTTP_GET, GetDisplayModuleHandler);
    m_Handlers.push_back(GetDisplayModuleHandler);
}

void HTTPServer::AddAPISetDisplayModule() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetDisplayModule");
    auto SetDisplayModuleHandler = new PsychicWebHandler();
    SetDisplayModuleHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received SetDisplayModule request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("displayModule")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing Display Module");
        }

        PsychicWebParameter* pDisplayModule = request->getParam("displayModule");
        if (!Config::Set(Config::Key::DisplayDriver, pDisplayModule->value().c_str())) {
            Log::Error("[HTTPServer] Invalid Display Module");
            return request->reply(400, "text/plain", "Invalid Display Module");
        }

        Log::Debug("[HTTPServer] Display Module set to %s", pDisplayModule->value().c_str());
        Config::Save();
        return request->reply(200, "text/plain", "Display Module set. Restart is needed to apply changes.");
    });
    m_MainServer->on("/api/v1/SetDisplayModule", HTTP_POST, SetDisplayModuleHandler);
    m_Handlers.push_back(SetDisplayModuleHandler);
}

void HTTPServer::AddAPIGetOpMode() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetOpMode");
    auto GetOpModeHandler = new PsychicWebHandler();
    GetOpModeHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received GetOpMode request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string opMode = Config::Get(Config::Key::OperatingMode);
        return request->reply(200, "text/plain", opMode.c_str());
    });
    m_MainServer->on("/api/v1/GetOpMode", HTTP_GET, GetOpModeHandler);
    m_Handlers.push_back(GetOpModeHandler);
}

void HTTPServer::AddAPIGetDisplayWidth() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetDisplayWidth");
    auto GetDisplayWidthHandler = new PsychicWebHandler();
    GetDisplayWidthHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received GetDisplayWidth request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string displayWidth = std::to_string(EPDL::GetWidth());
        return request->reply(200, "text/plain", displayWidth.c_str());
    });
    m_MainServer->on("/api/v1/GetDisplayWidth", HTTP_GET, GetDisplayWidthHandler);
    m_Handlers.push_back(GetDisplayWidthHandler);
}

void HTTPServer::AddAPIGetDisplayHeight() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetDisplayHeight");
    auto GetDisplayHeightHandler = new PsychicWebHandler();
    GetDisplayHeightHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received GetDisplayHeight request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string displayHeight = std::to_string(EPDL::GetHeight());
        return request->reply(200, "text/plain", displayHeight.c_str());
    });
    m_MainServer->on("/api/v1/GetDisplayHeight", HTTP_GET, GetDisplayHeightHandler);
    m_Handlers.push_back(GetDisplayHeightHandler);
}

void HTTPServer::AddAPIls() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/ls");
    auto SetlsHandler = new PsychicWebHandler();
    SetlsHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received ls request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("path")) {
            Log::Debug("Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing path");
        }

        PsychicWebParameter* pPath = request->getParam("path");
        std::string ls = MCU::Filesystem::ls(pPath->value().c_str());

        return request->reply(400, "text/plain", ls.c_str());
    });
    m_MainServer->on("/api/v1/ls", HTTP_POST, SetlsHandler);
    m_Handlers.push_back(SetlsHandler);
}

void HTTPServer::AddAPIrm() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/rm");
    auto SetrmHandler = new PsychicWebHandler();
    SetrmHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received rm request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("path")) {
            Log::Debug("Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing path");
        }

        PsychicWebParameter* pPath = request->getParam("path");

        if (!MCU::Filesystem::rm(pPath->value().c_str())) {
            Log::Error("[HTTPServer]  Failed to remove file");
            return request->reply(400, "text/plain", "Failed to remove file");
        }

        return request->reply(400, "text/plain", "Successfully removed file");
    });
    m_MainServer->on("/api/v1/rm", HTTP_POST, SetrmHandler);
    m_Handlers.push_back(SetrmHandler);
}

void HTTPServer::AddAPImkdir() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/mkdir");
    auto SetmkdirHandler = new PsychicWebHandler();
    SetmkdirHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received mkdir request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("path")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing path");
        }

        PsychicWebParameter* pPath = request->getParam("path");

        if (!MCU::Filesystem::mkdir(pPath->value().c_str())) {
            Log::Error("[HTTPServer] Failed to create directory");
            return request->reply(400, "text/plain", "Failed to create directory");
        }

        return request->reply(400, "text/plain", "Successfully created directory");
    });
    m_MainServer->on("/api/v1/mkdir", HTTP_POST, SetmkdirHandler);
    m_Handlers.push_back(SetmkdirHandler);
}

void HTTPServer::AddAPIRestart() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/restart");
    auto SetRestartHandler = new PsychicWebHandler();
    SetRestartHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received restart request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        request->reply(200, "text/plain", "Restarting device in 3 seconds.");
        Config::Save();

        MCU::Sleep(3000);
        MCU::Restart();
        return ESP_OK; // Never reached
    });
    m_MainServer->on("/api/v1/restart", HTTP_POST, SetRestartHandler);
    m_Handlers.push_back(SetRestartHandler);
}

void HTTPServer::AddAPISetLogLevel() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetLogLevel");
    auto SetLogLevelHandler = new PsychicWebHandler();
    SetLogLevelHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received SetLogLevel request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("level")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing log level");
        }

        if (!Config::Set(Config::Key::LogLevel, request->getParam("level")->value().c_str())) {
            Log::Error("[HTTPServer] Invalid log level");
            return request->reply(400, "text/plain", "Invalid log level");
        }
        Log::SetLogLevel(Config::Get(Config::Key::LogLevel));
        Config::Save();
        return request->reply(200, "text/plain", "Log level set");
    });
    m_MainServer->on("/api/v1/SetLogLevel", HTTP_POST, SetLogLevelHandler);
    m_Handlers.push_back(SetLogLevelHandler);
}

void HTTPServer::AddAPIGetLogLevel() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetLogLevel");
    auto GetLogLevelHandler = new PsychicWebHandler();
    GetLogLevelHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received GetLogLevel request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string logLevel = Config::Get(Config::Key::LogLevel);
        return request->reply(200, "text/plain", logLevel.c_str());
    });
    m_MainServer->on("/api/v1/GetLogLevel", HTTP_GET, GetLogLevelHandler);
    m_Handlers.push_back(GetLogLevelHandler);
}
void HTTPServer::EnableHTTPAuth(std::string_view username, std::string_view password) {
    Log::Debug("[HTTPServer] Enabling HTTP Auth");
    for (auto handler: m_Handlers) {
        handler->setAuthentication(username.data(), password.data(), BASIC_AUTH, "ESP32", "Authentication failed");
    }
}

void HTTPServer::AddAPISetHTTPAuth() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetHTTPAuth");
    auto SetHTTPAuthHandler = new PsychicWebHandler();
    SetHTTPAuthHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received SetHTTPAuth request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("username") && !request->hasParam("password")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing username or password");
        }

        PsychicWebParameter* pUsername = request->getParam("username");
        PsychicWebParameter* pPassword = request->getParam("password");

        if (!Config::Set(Config::Key::HTTPUsername, pUsername->value().c_str())
            || !Config::Set(Config::Key::HTTPPassword, pPassword->value().c_str())) {
            Log::Error("[HTTPServer] Invalid HTTP credentials");
            return request->reply(400, "text/plain", "Invalid HTTP credentials");
        }

        Log::Debug("[HTTPServer] HTTP credentials set to %s, %s",
                   pUsername->value().c_str(),
                   pPassword->value().c_str());
        Config::Save();
        return request->reply(200, "text/plain", "HTTP credentials set. Restart is needed to apply changes.");
    });
    m_MainServer->on("/api/v1/SetHTTPAuth", HTTP_POST, SetHTTPAuthHandler);
    m_Handlers.push_back(SetHTTPAuthHandler);
}

void HTTPServer::AddAPISetHTTPS() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetHTTPS");
    auto SetHTTPSHandler = new PsychicWebHandler();
    SetHTTPSHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received SetHTTPS request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("https")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing HTTPS parameter");
        }

        PsychicWebParameter* pHTTPS = request->getParam("https");
        if (!Config::Set(Config::Key::HTTPS, pHTTPS->value().c_str())) {
            Log::Error("[HTTPServer] Invalid HTTPS parameter");
            return request->reply(400, "text/plain", "Invalid HTTPS parameter");
        }

        Log::Debug("[HTTPServer] HTTPS set to %s", pHTTPS->value().c_str());
        Config::Save();
        return request->reply(200, "text/plain", "HTTPS set. Restart is needed to apply changes.");
    });
    m_MainServer->on("/api/v1/SetHTTPS", HTTP_POST, SetHTTPSHandler);
    m_Handlers.push_back(SetHTTPSHandler);
}

void HTTPServer::AddAPIGetHTTPS() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetHTTPS");
    auto GetHTTPSHandler = new PsychicWebHandler();
    GetHTTPSHandler->onRequest([](PsychicRequest* request) {
        Log::Info("[HTTPServer] Received GetHTTPS request from client %s",
                  request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string https = Config::Get(Config::Key::HTTPS);
        return request->reply(200, "text/plain", https.c_str());
    });
    m_MainServer->on("/api/v1/GetHTTPS", HTTP_GET, GetHTTPSHandler);
    m_Handlers.push_back(GetHTTPSHandler);
}

void HTTPServer::AddAPIUploadHTTPSKey() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/UploadHTTPSKey");
    auto UploadHTTPSKeyHandler = new PsychicUploadHandler();
    UploadHTTPSKeyHandler->onUpload([](PsychicRequest* request, const String &filename, uint64_t index,
                                       uint8_t* data,
                                       size_t len,
                                       bool last) {
        bool* status;
        std::ofstream file;
        std::string path = "/server.key";
        std::string pathNative = MCU::Filesystem::GetPath(path);
        bool isFirst = index == 0;

        if (isFirst) {
            Log::Info("[HTTPServer] HTTPS Key upload started from client %s to %s",
                      request->client()->remoteIP().toString().c_str(),
                      pathNative.c_str());
            file.open(pathNative, std::ios::binary | std::ios::trunc);
            status = new bool(true);
            request->_tempObject = status;
        }
        else {
            status = static_cast<bool*>(request->_tempObject);
            if (!*status) { return ESP_FAIL; }
            file.open(pathNative, std::ios::binary | std::ios::app);
        }

        if (!file.is_open()) {
            Log::Error("[HTTPServer] Failed to open file");
            *status = false;
            return ESP_FAIL;
        }

        if (!file.write((char*) data, len)) {
            Log::Error("[HTTPServer] Write failed");
            *status = false;
            file.close();
            return ESP_FAIL;
        }

        file.close();

        Log::Debug("[HTTPServer] Wrote %d bytes", len);
        if (last) {
            Log::Debug("[HTTPServer] HTTPS Key upload finished");
        }

        return ESP_OK;
    });

    // Called after upload has been handled
    UploadHTTPSKeyHandler->onRequest([](PsychicRequest* request) {
        bool status = true;
        if (request->_tempObject) { status = *static_cast<bool*>(request->_tempObject); }
        delete static_cast<bool*>(request->_tempObject);
        request->_tempObject = nullptr;

        if (!status) {
            Log::Error("[HTTPServer] HTTPS Key upload failed");
            return request->reply(500, "text/plain", "HTTPS Key upload failed");
        }

        Log::Info("[HTTPServer] HTTPS Key upload successful");
        return request->reply(200, "text/plain", "HTTPS Key successfully uploaded");
    });

    m_MainServer->on("/api/v1/UploadHTTPSKey", HTTP_POST, UploadHTTPSKeyHandler);
    m_Handlers.push_back(UploadHTTPSKeyHandler);
}

void HTTPServer::AddAPIUploadHTTPSCert() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/UploadHTTPSCert");
    auto UploadHTTPSCertHandler = new PsychicUploadHandler();
    UploadHTTPSCertHandler->onUpload([](PsychicRequest* request, const String &filename, uint64_t index,
                                        uint8_t* data,
                                        size_t len,
                                        bool last) {
        bool* status;
        std::ofstream file;
        std::string path = "/server.crt";
        std::string pathNative = MCU::Filesystem::GetPath(path);
        bool isFirst = index == 0;

        if (isFirst) {
            Log::Info("[HTTPServer] HTTPS Cert upload started from client %s to %s",
                      request->client()->remoteIP().toString().c_str(),
                      pathNative.c_str());
            file.open(pathNative, std::ios::binary | std::ios::trunc);
            status = new bool(true);
            request->_tempObject = status;
        }
        else {
            status = static_cast<bool*>(request->_tempObject);
            if (!*status) { return ESP_FAIL; }
            file.open(pathNative, std::ios::binary | std::ios::app);
        }

        if (!file.is_open()) {
            Log::Error("[HTTPServer] Failed to open file");
            *status = false;
            return ESP_FAIL;
        }

        if (!file.write((char*) data, len)) {
            Log::Error("[HTTPServer] Write failed");
            *status = false;
            file.close();
            return ESP_FAIL;
        }

        file.close();

        Log::Debug("[HTTPServer] Wrote %d bytes", len);
        if (last) {
            Log::Debug("[HTTPServer] HTTPS Cert upload finished");
        }

        return ESP_OK;
    });

    // Called after upload has been handled
    UploadHTTPSCertHandler->onRequest([](PsychicRequest* request) {
        bool status = true;
        if (request->_tempObject) { status = *static_cast<bool*>(request->_tempObject); }
        delete static_cast<bool*>(request->_tempObject);
        request->_tempObject = nullptr;

        if (!status) {
            Log::Error("[HTTPServer] HTTPS Cert upload failed");
            return request->reply(500, "text/plain", "HTTPS Cert upload failed");
        }

        Log::Info("[HTTPServer] HTTPS Cert upload successful");
        return request->reply(200, "text/plain", "HTTPS Cert successfully uploaded");
    });

    m_MainServer->on("/api/v1/UploadHTTPSCert", HTTP_POST, UploadHTTPSCertHandler);
    m_Handlers.push_back(UploadHTTPSCertHandler);
}
