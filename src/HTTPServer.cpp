#include <Arduino.h>
#include <ArduinoJson.h>
#include <fstream>
#include <LittleFS.h>
#include <memory>
#include <WiFi.h>
#include "Config.h"
#include <EPDL.h>
#include "HTTPServer.h"
#include <Log.h>


void HTTPServer::Init(int* RenderIMG) {
    Log::Debug("Initializing HTTP server");
    m_Server.listen(80);
    m_Server.config.max_uri_handlers = 20;

    m_Server.serveStatic("/", LittleFS, "/www/");

    m_Server.on("/ip", HTTP_GET, [](PsychicRequest* request) {
        String output = "Your IP is: " + request->client()->remoteIP().toString();
        return request->reply(output.c_str());
    });

    m_Server.on("/api/v1/SetAPCred", HTTP_POST, [](PsychicRequest* request) {
        Log::Debug("Received SetAPCred request from client %s",
                   request->client()->remoteIP().toString().c_str());

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


    if (!LittleFS.exists("/upload")) {
        Log::Debug("Creating upload directory");
        if (!LittleFS.mkdir("/upload")) { Log::Error("Failed to create upload directory"); }
    }

    // m_Server.on("/api/v1/upload", HTTP_POST,
    //     [](PsychicRequest* request) {
    //         request->reply(200);
    //     },
    //     [](PsychicRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
    //         if (index == 0) { // index zero means first chunk of file
    //             Log::Debug("Received file upload request from client %s", request->client()->remoteIP().toString().c_str());
    //             Log::Debug("Upload Started: %s", filename.c_str());
    //         }
    //         Log::Debug("File upload: %s, index: %u, len: %u, final: %s", filename.c_str(), index, len, final ? "true" : "false");


    //         std::ofstream file("/littlefs/upload/" + std::string(filename.c_str()),
    //             index == 0 ? std::ios::binary : std::ios::binary | std::ios::app);

    //         if (file) {
    //             if (len > 0) {
    //                 file.write(reinterpret_cast<const char*>(data), len);
    //             }
    //         }
    //         else {
    //             Log::Error("Failed to open file for writing");
    //             request->reply(500, "text/plain", "Failed to open file for writing");
    //         }

    //         if (final) {
    //             Log::Debug("Upload Ended: %s", filename.c_str());
    //             request->reply(200, "text/plain", "File uploaded successfully");
    //         }
    //         else {
    //             request->reply(200, "text/plain", "File chunk uploaded successfully");
    //         }

    //         file.close();
    //     });

    // AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/v1/uploadbmp", [RenderIMG](PsychicRequest* request, JsonVariant& json) {
    //     Log::Debug("Received /api/v1/uploadbmp");
    //
    //     std::unique_ptr<EPDL::ImageData> img = std::make_unique<EPDL::ImageData>();
    //     img->Height = json["height"];
    //     img->Width = json["width"];
    //
    //     JsonArray data = json["data"];
    //     for (JsonVariant value : data) {
    //         img->Data.push_back(value.as<uint8_t>());
    //     }
    //
    //     { // Debug
    //         Log::Debug("Image data: %u x %u", img->Width, img->Height);
    //         Log::Debug("Data: %u", img->Data.size());
    //     }
    //
    //     *RenderIMG = EPDL::CreateImage(std::move(img));
    //
    //     request->reply(200, "application/json", "{\"result\":\"success\"}");
    //
    //     });
    // m_Server.addHandler(handler);


    // m_Server.on("/api/v1/uploadbmp", HTTP_POST, [RenderIMG](PsychicRequest* request, JsonVariant& json) {
    //     Log::Debug("Received /api/v1/uploadbmp");
    //
    //     std::unique_ptr<EPDL::ImageData> img = std::make_unique<EPDL::ImageData>();
    //     img->Height = json["height"];
    //     img->Width = json["width"];
    //
    //     JsonArray data = json["data"];
    //     for (JsonVariant value : data) { img->Data.push_back(value.as<uint8_t>()); }
    //
    //     { // Debug
    //         Log::Debug("Image data: %u x %u", img->Width, img->Height);
    //         Log::Debug("Data: %u", img->Data.size());
    //     }
    //
    //     *RenderIMG = EPDL::CreateImage(std::move(img));
    //
    //     return request->reply(200, "application/json", "{\"result\":\"success\"}");
    // });

    // PsychicUploadHandler* multipartHandler = new PsychicUploadHandler();
    // multipartHandler->onUpload(
    //     [this](PsychicRequest* request, const String& filename, uint64_t index, uint8_t* data, size_t len, bool last) {
    //         std::string identifier = request->client()->remoteIP().toString().c_str();
    //         for (uint8_t i = 0; i < len; i++)
    //             this->uploadBuffers[identifier].push_back(data[i]);

    //         Log::Debug("Saving %d bytes to the buffer", len);
    //         if (last)
    //             Log::Debug("Upload finished. Total bytes: %d", this->uploadBuffers[identifier].size());

    //         return ESP_OK;
    //     });

    // //gets called after upload has been handled
    // multipartHandler->onRequest([this, RenderIMG](PsychicRequest* request) {
    //     PsychicWebParameter* width = request->getParam("width");
    //     PsychicWebParameter* height = request->getParam("height");

    //     std::unique_ptr<EPDL::ImageData> img = std::make_unique<EPDL::ImageData>();
    //     img->Width = width->value().toInt();
    //     img->Height = height->value().toInt();
    //     img->Data = this->uploadBuffers[request->client()->remoteIP().toString().c_str()];

    //     *RenderIMG = EPDL::CreateImage(std::move(img));

    //     this->uploadBuffers[request->client()->remoteIP().toString().c_str()].clear();

    //     return request->reply(200, "text/plain", "Bitmap uploaded successfully");
    // });

    // //upload to /multipart url
    // m_Server.on("/api/v1/uploadbmp", HTTP_POST, multipartHandler);

    m_Server.onNotFound([](PsychicRequest* request) {
        Log::Debug("Not found: %s", request->url());
        return request->reply(404, "text/plain", "Page not found");
    });

    Log::Info("HTTP server started");
}
