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

    m_Server.on("/api/v1/ConfigOpMode", HTTP_POST, [](AsyncWebServerRequest* request) {
        Log::Debug("Received ConfigOpMode request from client %s", request->client()->remoteIP().toString().c_str());

        AsyncWebParameter* pOpMode = request->getParam("opmode", true);

        if (pOpMode) {
            Config::SetOperatingMode(pOpMode->value().c_str());
            Config::SaveConfig();
            request->send(200, "text/plain", " Operating Mode set. Restarting device in 3 seconds.");
            delay(3000);
            ESP.restart();
        }
        else {
            Log::Debug("Invalid request, missing parameters");
            request->send(400, "text/plain", "Missing Operating Mode");
            return;
        }
        });

    m_Server.on("/api/v1/SetAPCred", HTTP_POST, [](AsyncWebServerRequest* request) {
        Log::Debug("Received SetAPCred request from client %s", request->client()->remoteIP().toString().c_str());

        AsyncWebParameter* pSSID = request->getParam("ssid", true);
        AsyncWebParameter* pPassword = request->getParam("password", true);

        if (pSSID && pPassword) {
            Config::SetWiFiSSID(pSSID->value().c_str());
            Config::SetWiFiPassword(pPassword->value().c_str());
            Config::SaveConfig();
            request->send(200, "text/plain", "WiFi credentials set. Please restart the device.");
        }
        else {
            Log::Debug("Invalid request, missing parameters");
            request->send(400, "text/plain", "Missing SSID or password");
            return;
        }
        });


    if (!LittleFS.exists("/upload")) {
        Log::Debug("Creating upload directory");
        if (!LittleFS.mkdir("/upload")) {
            Log::Error("Failed to create upload directory");
        }
    }

    m_Server.on("/api/v1/upload", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            request->send(200);
        },
        [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) { // index zero means first chunk of file
                Log::Debug("Received file upload request from client %s", request->client()->remoteIP().toString().c_str());
                Log::Debug("Upload Started: %s", filename.c_str());
            }
            Log::Debug("File upload: %s, index: %u, len: %u, final: %s", filename.c_str(), index, len, final ? "true" : "false");


            std::ofstream file("/littlefs/upload/" + std::string(filename.c_str()),
                index == 0 ? std::ios::binary : std::ios::binary | std::ios::app);

            if (file) {
                if (len > 0) {
                    file.write(reinterpret_cast<const char*>(data), len);
                }
            }
            else {
                Log::Error("Failed to open file for writing");
                request->send(500, "text/plain", "Failed to open file for writing");
            }

            if (final) {
                Log::Debug("Upload Ended: %s", filename.c_str());
                request->send(200, "text/plain", "File uploaded successfully");
            }
            else {
                request->send(200, "text/plain", "File chunk uploaded successfully");
            }

            
            file.close();
        });


    m_Server.onNotFound([](AsyncWebServerRequest* request) {
        Log::Debug("Not found: %s", request->url());
        request->send(404);
        });

    m_Server.begin();
    Log::Info("HTTP server started");
}
