#include <Arduino.h>
#include <WiFi.h>
#include <fstream>
#include <LittleFS.h>
#include "Config.h"
#include "HTTPServer.h"
#include <Log.h>
#include <MCU.h>
#include <Filesystem.h>
#include <EPDL.h>

void HTTPServer::Init() {
    Log::Debug("[HTTPServer] Initializing HTTP server");
    m_Server.listen(80);
    m_Server.config.max_uri_handlers = 20;
    m_Server.onNotFound([](PsychicRequest* request) {
        Log::Debug("Not found: %s", request->url().c_str());
        return request->reply(404, "text/plain", "Page not found");
    });

    Log::Info("[HTTPServer] HTTP server initialized");
}

void HTTPServer::SetFilesToServe(const std::map<std::string, std::string> &files) {
    Log::Debug("[HTTPServer] Setting files to serve");
    for (const auto &file: files) {
        Log::Debug("[HTTPServer] Adding file %s to serve from %s", file.second.c_str(), file.first.c_str());
        PsychicStaticFileHandler* handler = new PsychicStaticFileHandler(file.first.c_str(),
                                                                         LittleFS,
                                                                         file.second.c_str(),
                                                                         nullptr);
        m_Server.addHandler(handler);
    }
    m_Server.on("/", HTTP_GET, [](PsychicRequest* request) {
        return request->redirect("/index.html");
    });
}

void HTTPServer::AddAPISetOpMode() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetOpMode");
    PsychicWebHandler* SetOpModeHandler = new PsychicWebHandler();
    SetOpModeHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received ConfigOpMode request from client %s",
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
        request->reply(200, "text/plain", "Operating Mode set. Restarting device in 3 seconds.");
        MCU::Sleep(3000);
        MCU::Restart();
        return ESP_OK;
    });
    m_Server.on("/api/v1/SetOpMode", HTTP_POST, SetOpModeHandler);
}

void HTTPServer::AddAPISetWiFiCred() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetWiFiCred");
    PsychicWebHandler* SetWiFiCredHandler = new PsychicWebHandler();
    SetWiFiCredHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received SetWiFiCred request from client %s",
                   request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        if (!request->hasParam("ssid") && !request->hasParam("password")) {
            Log::Debug("[HTTPServer] Invalid request, missing parameters");
            return request->reply(400, "text/plain", "Missing SSID or password");
        }

        PsychicWebParameter* pSSID = request->getParam("ssid");
        PsychicWebParameter* pPassword = request->getParam("password");

        if (!Config::Set(Config::Key::WiFiSSID, pSSID->value().c_str()) &&
            !Config::Set(Config::Key::WiFiPassword, pPassword->value().c_str())) {
            Log::Error("[HTTPServer] Invalid WiFi credentials");
            return request->reply(400, "text/plain", "Invalid WiFi credentials");
        }

        Log::Debug("[HTTPServer] WiFi credentials set to %s", pSSID->value().c_str());
        Config::Save();
        return request->reply(200, "text/plain", "WiFi credentials set. Please restart the device.");
    });
    m_Server.on("/api/v1/SetWiFiCred", HTTP_POST, SetWiFiCredHandler);
}

void HTTPServer::AddAPIUploadImg(std::function<void(std::string_view filePath)> onUpload) {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/UploadImg");
    if (!MCU::Filesystem::exists("/upload")) { MCU::Filesystem::mkdir("/upload"); }

    PsychicUploadHandler* UploadImgHandler = new PsychicUploadHandler();
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
            Log::Debug("[HTTPServer] Image upload started from client %s to %s",
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

        Log::Debug("[HTTPServer] Image upload successful");
        return request->reply(200, "text/plain", "Image successfully uploaded");
    });

    m_Server.on("/api/v1/UploadImg", HTTP_POST, UploadImgHandler);
}

void HTTPServer::AddAPIGetDisplayModule() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetDisplayModule");
    PsychicWebHandler* GetDisplayModuleHandler = new PsychicWebHandler();
    GetDisplayModuleHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received GetDisplayModule request from client %s",
                   request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string displayModule = Config::Get(Config::Key::DisplayDriver);
        return request->reply(200, "text/plain", displayModule.c_str());;
    });
    m_Server.on("/api/v1/GetDisplayModule", HTTP_GET, GetDisplayModuleHandler);
}

void HTTPServer::AddAPISetDisplayModule() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/SetDisplayModule");
    PsychicWebHandler* SetDisplayModuleHandler = new PsychicWebHandler();
    SetDisplayModuleHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received SetDisplayModule request from client %s",
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
        return request->reply(200, "text/plain", "Display Module set. Please restart the device.");
    });
    m_Server.on("/api/v1/SetDisplayModule", HTTP_POST, SetDisplayModuleHandler);
}

void HTTPServer::AddAPIGetOpMode() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetOpMode");
    PsychicWebHandler* GetOpModeHandler = new PsychicWebHandler();
    GetOpModeHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received GetOpMode request from client %s",
                   request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string opMode = Config::Get(Config::Key::OperatingMode);
        return request->reply(200, "text/plain", opMode.c_str());
    });
    m_Server.on("/api/v1/GetOpMode", HTTP_GET, GetOpModeHandler);
}

void HTTPServer::AddAPIGetDisplayWidth() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetDisplayWidth");
    PsychicWebHandler* GetDisplayWidthHandler = new PsychicWebHandler();
    GetDisplayWidthHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received GetDisplayWidth request from client %s",
                   request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string displayWidth = std::to_string(EPDL::GetWidth());
        return request->reply(200, "text/plain", displayWidth.c_str());
    });
    m_Server.on("/api/v1/GetDisplayWidth", HTTP_GET, GetDisplayWidthHandler);
}

void HTTPServer::AddAPIGetDisplayHeight() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/GetDisplayHeight");
    PsychicWebHandler* GetDisplayHeightHandler = new PsychicWebHandler();
    GetDisplayHeightHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received GetDisplayHeight request from client %s",
                   request->client()->remoteIP().toString().c_str());
        Log::Trace("[HTTPServer] Body: %s", request->body().c_str());

        std::string displayHeight = std::to_string(EPDL::GetHeight());
        return request->reply(200, "text/plain", displayHeight.c_str());
    });
    m_Server.on("/api/v1/GetDisplayHeight", HTTP_GET, GetDisplayHeightHandler);
}

void HTTPServer::AddAPIls() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/ls");
    PsychicWebHandler* SetlsHandler = new PsychicWebHandler();
    SetlsHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received ls request from client %s",
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
    m_Server.on("/api/v1/ls", HTTP_POST, SetlsHandler);
}

void HTTPServer::AddAPIrm() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/rm");
    PsychicWebHandler* SetrmHandler = new PsychicWebHandler();
    SetrmHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received rm request from client %s",
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
    m_Server.on("/api/v1/rm", HTTP_POST, SetrmHandler);
}

void HTTPServer::AddAPImkdir() {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/mkdir");
    PsychicWebHandler* SetmkdirHandler = new PsychicWebHandler();
    SetmkdirHandler->onRequest([](PsychicRequest* request) {
        Log::Debug("[HTTPServer] Received mkdir request from client %s",
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
    m_Server.on("/api/v1/mkdir", HTTP_POST, SetmkdirHandler);
}