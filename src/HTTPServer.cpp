#include <Arduino.h>
#include <WiFi.h>
#include <fstream>
#include <HTTPServer.h>
#include "Log.h"
#include "Config.h"

HTTPServer::HTTPServer() : m_Server(80) {
}

void HTTPServer::Init() {
    Log::Debug("Starting HTTP server");

    m_Server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        std::ifstream file("/littlefs/html/index.html", std::ios::in | std::ios::binary);
        if (file) {
            Log::Debug("Serving file: /littlefs/html/index.html");
            std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            request->send(200, "text/html", contents.c_str());
            file.close();
        }
        else {
            Log::Debug("File not found: /html/index.html");
            request->send(404);
        }
        });

    m_Server.on("^\\/(.*)\\.html$", HTTP_GET, [](AsyncWebServerRequest* request) {
        String path = "/littlefs/html" + request->url();
        std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (file) {
            Log::Debug("Serving file: %s", path.c_str());
            std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            request->send(200, "text/html", contents.c_str());
            file.close();
        }
        else {
            Log::Debug("File not found: %s", path.c_str());
            request->send(404);
        }
        });

    m_Server.on("/api/SetAPCred", HTTP_POST, [](AsyncWebServerRequest* request) {
        Log::Debug("Received SetAPCred request from client %s", request->client()->remoteIP().toString().c_str());

        AsyncWebParameter* pSSID = request->getParam("ssid", true);
        AsyncWebParameter* pPassword = request->getParam("password", true);

        if (pSSID && pPassword) {
            Config::GetInstance().SetWiFiSSID(pSSID->value().c_str());
            Config::GetInstance().SetWiFiPassword(pPassword->value().c_str());
            Config::SaveConfig();
            request->send(200, "text/plain", "WiFi credentials set. Please restart the device.");
        }
        else {
            Log::Debug("Invalid request, missing parameters");
            request->send(400, "text/plain", "Missing SSID or password");
            return;
        }
        });

    m_Server.onNotFound([](AsyncWebServerRequest* request) {
        Log::Debug("Not found: %s", request->url());
        request->send(404);
        });

    m_Server.begin();
    Log::Info("HTTP server started");
    Log::Debug("Listening on: %s", WiFi.softAPIP().toString());
}
