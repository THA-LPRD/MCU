#include <Arduino.h>
#include <LittleFS.h>
#include <string>
#include "Application.h"
#include "Log.h"
#include <Clock.h>
#include "Config.h"
#include <GPIO.h>
#include <MCU.h>
#include <Filesystem.h>
#include <fstream>

void FuncLog(char* msg) {
    time_t time = MCU::Clock::GetTime();
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

    MCU::Clock::SetTime(2024, 1, 1, 0, 0, 0, -1);

    Log::SetLogFunction(FuncLog);
    Log::SetLogLevel(Log::Level::DEBUG);

    Log::Debug("Starting File System");
    MCU::Filesystem::Init();

    Config::Load();
    EPDL::Init();
    EPDL::LoadDriver(Config::Get(Config::Key::DisplayDriver));

    if (MCU::GPIO::Read(Config::Pin::RST) == 0) {
        Log::Info("Reset button pressed. Loading default configuration.");
        Config::Set(Config::Key::OperatingMode, "Default");
        Config::Save();
    }
    else {
        Log::Debug("Reset button not pressed. Continuing Setup.");
    }

    app = Application::Create(Config::Get(Config::Key::OperatingMode));
    if (app->Init()) { return; }

    Log::Fatal("Failed to initialize application. Starting default app");
    delete app;
    Config::LoadDefault();
    app = Application::Create("Default");
    if (app->Init()) { exit(1); }
}

void loop() {
    app->Run();
}
