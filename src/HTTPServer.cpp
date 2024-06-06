#include <Arduino.h>
#include <WiFi.h>
#include <fstream>
#include <LittleFS.h>
#include "Config.h"
#include "HTTPServer.h"
#include <Log.h>
#include <MCU.h>
#include <Filesystem.h>

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

        if (request->hasParam("opmode")) {
            PsychicWebParameter* pOpMode = request->getParam("opmode");
            if (Config::Set(Config::Key::OperatingMode, pOpMode->value().c_str())) {
                Log::Debug("Operating Mode set to %s", pOpMode->value().c_str());
                Config::Save();
                request->reply(200, "text/plain", "Operating Mode set. Restarting device in 3 seconds.");
                MCU::Sleep(3000);
                MCU::Restart();
            }
            else {
                Log::Error("Invalid Operating Mode");
                return request->reply(400, "text/plain", "Invalid Operating Mode");
            }
        }

        Log::Debug("Invalid request, missing parameters");
        return request->reply(400, "text/plain", "Missing Operating Mode");
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

        if (request->hasParam("ssid") && request->hasParam("password")) {
            PsychicWebParameter* pSSID = request->getParam("ssid");
            PsychicWebParameter* pPassword = request->getParam("password");

            if (Config::Set(Config::Key::WiFiSSID, pSSID->value().c_str()) &&
                Config::Set(Config::Key::WiFiPassword, pPassword->value().c_str())) {
                Log::Debug("[HTTPServer] WiFi credentials set to %s", pSSID->value().c_str());
                Config::Save();
                return request->reply(200, "text/plain", "WiFi credentials set. Please restart the device.");
            }
            else {
                Log::Error("[HTTPServer] Invalid WiFi credentials");
                return request->reply(400, "text/plain", "Invalid WiFi credentials");
            }
        }

        Log::Debug("[HTTPServer] Invalid request, missing parameters");
        return request->reply(400, "text/plain", "Missing SSID or password");
    });
    m_Server.on("/api/v1/SetWiFiCred", HTTP_POST, SetWiFiCredHandler);
}

void HTTPServer::AddAPIUploadImg(std::function<void(std::string_view filePath)> onUpload) {
    Log::Debug("[HTTPServer] Adding API endpoint /api/v1/UploadImg");
    PsychicUploadHandler* UploadImgHandler = new PsychicUploadHandler();
    UploadImgHandler->onUpload([onUpload](PsychicRequest* request, const String &filename, uint64_t index,
                                          uint8_t* data,
                                          size_t len,
                                          bool last) {
        bool* status;
        std::ofstream file;
        std::string path = MCU::Filesystem::GetPath("/upload/");
        path += filename.c_str();
        bool isFirst = index == 0;

        if (isFirst) {
            Log::Debug("[HTTPServer] Image upload started from client %s to %s",
                       request->client()->remoteIP().toString().c_str(),
                       path.c_str());
            file.open(path, std::ios::binary | std::ios::trunc);
            status = new bool(true);
            request->_tempObject = status;
        }
        else {
            status = static_cast<bool*>(request->_tempObject);
            if (!*status) { return ESP_FAIL; }
            file.open(path, std::ios::binary | std::ios::app);
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
