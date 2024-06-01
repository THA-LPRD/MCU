#include <Arduino.h>
#include <LittleFS.h>
#include <string>
#include "Application.h"
#include "Log.h"
#include "Clock.h"
#include "Config.h"
#include <GPIO.h>
#include <MCU.h>

void FuncLog(char* msg) {
    time_t time = Clock::GetTime();
    struct tm* timeinfo = localtime(&time);
    char time_str[18];
    strftime(time_str, 18, "%y-%m-%d %H:%M:%S", timeinfo);
    std::string log_msg = "[" + std::string(time_str) + "] - " + std::string(msg);
    Serial.println(log_msg.c_str());
}
Application* app = nullptr;

void setup() {
    Serial.begin(115200);

    MCU::GPIO::SetMode(Config::Pin::RST, MCU::GPIO::Mode::InputPullup);

    Clock::SetTime(2024, 1, 1, 0, 0, 0, -1);

    Log::SetLogFunction(FuncLog);
    Log::SetLogLevel(Log::Level::DEBUG);

    Log::Debug("Starting File System");
    if (!LittleFS.begin(true)) {
        Log::Fatal("Failed to start file system");
        return;
    }

    Config::LoadConfig();
    EPDL::Init();
    EPDL::LoadDriver(Config::GetDisplayDriver());


    if (MCU::GPIO::Read(Config::Pin::RST) == 1) {
        Log::Info("Reset button pressed. Loading default configuration.");
        Config::SetOperatingMode("Default");
        Config::SaveConfig();
    }
    else {
        Log::Debug("Reset button not pressed. Continuing Setup.");
    }

    app = Application::Create(Config::GetOperatingMode());
    if (app == nullptr) {
        Log::Fatal("Failed to create application");
        MCU::Restart();
    }

    if (app->Init()) {
        return;
    }

    Log::Fatal("Failed to initialize application. Starting default app");
    delete app;
    Config::LoadDefaultConfig();
    app = Application::Create("Default");
    if (app == nullptr) {
        Log::Fatal("Failed to create default application.");
        MCU::Restart();
    }
}

void loop() {
    app->Run();
}
