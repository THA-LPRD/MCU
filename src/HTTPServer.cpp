#include <Arduino.h>
#include <WiFi.h>
#include <fstream>
#include <LittleFS.h>
#include "Config.h"
#include "HTTPServer.h"
#include <Log.h>
#include <MCU.h>

void HTTPServer::Init() {
    Log::Debug("Initializing HTTP server");

    m_Server.listen(80);
    m_Server.config.max_uri_handlers = 20;

    m_Server.serveStatic("/", LittleFS, "/www/");

    m_Server.on("/api/v1/ConfigOpMode", HTTP_POST, [](PsychicRequest* request) {
        Log::Debug("Received ConfigOpMode request from client %s", request->client()->remoteIP().toString().c_str());

        if (request->hasParam("opmode")) {
            PsychicWebParameter* pOpMode = request->getParam("opmode");
            Config::SetOperatingMode(pOpMode->value().c_str());
            Config::SaveConfig();
            request->reply(200, "text/plain", "Operating Mode set. Restarting device in 3 seconds.");
            MCU::Sleep(3000);
            MCU::Restart();
        }
        Log::Debug("Invalid request, missing parameters");
        return request->reply(400, "text/plain", "Missing Operating Mode");
    });

    m_Server.on("/api/v1/SetAPCred", HTTP_POST, [](PsychicRequest* request) {
        Log::Debug("Received SetAPCred request from client %s", request->client()->remoteIP().toString().c_str());

        if (request->hasParam("ssid") && request->hasParam("password")) {
            PsychicWebParameter* pSSID = request->getParam("ssid");
            PsychicWebParameter* pPassword = request->getParam("password");
            Config::SetWiFiSSID(pSSID->value().c_str());
            Config::SetWiFiPassword(pPassword->value().c_str());
            Config::SaveConfig();
            Log::Debug("SSID: %s, Password: %s", Config::GetWiFiSSID().c_str(),
                       Config::GetWiFiPassword().c_str());
            return request->reply(200, "text/plain", "WiFi credentials set. Please restart the device.");
        }
        Log::Debug("Invalid request, missing parameters");
        return request->reply(400, "text/plain", "Missing SSID or password");
    });

    m_Server.onNotFound([](PsychicRequest* request) {
        Log::Debug("Not found: %s", request->url().c_str());
        return request->reply(404, "text/plain", "Page not found");
    });

    Log::Info("HTTP server started");
}
