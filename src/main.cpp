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
    MCU::Sleep(1000);
    Serial.begin(115200);
    MCU::Clock::SetTime(2024, 1, 1, 0, 0, 0, -1);
    Log::SetLogFunction(FuncLog);
    Log::SetLogLevel(Log::Level::DEBUG);
    MCU::Sleep(5000);
    Log::Debug("Starting Serial");
    Log::Debug("Starting Serial");
    Log::Debug("Starting Serial");
    MCU::Sleep(1000);
    Log::Debug("Starting Serial");
    
    // MCU::GPIO::SetMode(Config::Pin::RST, MCU::GPIO::Mode::InputPullup);
    MCU::GPIO::SetMode(MCU::GPIO::BTN1, MCU::GPIO::Mode::InputPullup);

    Log::Debug("Starting File System");
    /*
    MCU::Filesystem::Init();

    Config::Load();

    // If Custom Button 1 is pressed on bootprocess, then reset to factory settings. 
    if (MCU::GPIO::Read(MCU::GPIO::BTN1) == 1) {
        Log::Info("Reset button pressed. Loading default configuration.");
        Config::Set(Config::Key::OperatingMode, "Default");
        Config::Save();
    }
    else {
        Log::Debug("Reset button not pressed. Continuing Setup.");
    }
    */
    // Power Peripherie on
    MCU::GPIO::SetMode(MCU::GPIO::VCC, MCU::GPIO::Mode::Output);
    MCU::GPIO::Write(MCU::GPIO::VCC, 1);

    // EPD Part
    EPDL::Init(Config::Get(Config::Key::DisplayDriver));
    EPDL::LoadDriver(Config::Get(Config::Key::DisplayDriver));

    

    app = Application::Create(Config::Get(Config::Key::OperatingMode));

    if (app->Init()) { return; }

    Log::Fatal("Failed to initialize application. Starting default app");
    delete app;
    Config::LoadDefault();
    app = Application::Create("Default");
}

void loop() {
    app->Run();
}
