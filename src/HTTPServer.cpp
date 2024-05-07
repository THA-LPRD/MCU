#include <Arduino.h>
#include <WiFi.h>
#include <fstream>
#include <LittleFS.h>
#include <HTTPServer.h>
#include "Log.h"
#include "Config.h"

HTTPServer::HTTPServer() : m_Server(80) {
}


void HTTPServer::Init() {
    Log::Debug("Initializing HTTP server");

    m_Server.serveStatic("/", LittleFS, "/www/");
    m_Server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

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
}
