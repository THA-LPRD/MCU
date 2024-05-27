#include <Arduino.h>
#include <LittleFS.h>
#include <string>
#include "Application.h"
#include "Log.h"
#include "Clock.h"
#include "Config.h"

#define RESET_BUTTON_PIN 2

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
    
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP); 

    Clock::SetTime(2024, 1, 1, 0, 0, 0, -1);

    Log::SetLogFunction(FuncLog);
    Log::SetLogLevel(Log::Level::DEBUG);

    Log::Debug("Starting File System");
    if (!LittleFS.begin(true)) {
        Log::Fatal("Failed to start file system");
        return;
    }

    Config::LoadConfig();

    if (digitalRead(RESET_BUTTON_PIN) == HIGH) {
        Log::Info("Reset button pressed. Loading default configuration.");
        Config::LoadDefaultConfig;
        return;
    }
    else 
        Log::Debug("Reset button not pressed. Continuing Setup.");

    app = Application::Create(Config::GetOperatingMode());
    if (app == nullptr) {
        Log::Fatal("Failed to create application");
        return;
    }

    if (app->Init()) {
        return;
    }

    Log::Fatal("Failed to initialize application. Starting default app");
    delete app;
    Application* app = Application::Create("Default");
    if (app == nullptr) {
        Log::Fatal("Failed to create default application.");
    }
    return;
}

void loop() {
    app->Run();
}
