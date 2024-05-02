#include <Arduino.h>
#include <LittleFS.h>
#include <string>
#include "Application.h"
#include "Log.h"
#include "Clock.h"
#include "Config.h"

void FuncLog(char* msg) {
    time_t time = Clock::GetTime();
    struct tm* timeinfo = localtime(&time);
    char time_str[18];
    strftime(time_str, 18, "%y-%m-%d %H:%M:%S", timeinfo);
    std::string log_msg = "[" + std::string(time_str) + "] - " + std::string(msg);
    Serial.println(log_msg.c_str());
}

void setup() {
    Serial.begin(115200);

    Clock::SetTime(2024, 1, 1, 0, 0, 0, -1);

    Log::SetLogFunction(FuncLog);
    Log::SetLogLevel(Log::Level::DEBUG);

    Log::Debug("Starting File System");
    if (!LittleFS.begin(true)) {
        Log::Fatal("Failed to start file system");
        return;
    }
    Log::Info("Starting application");
    static Application app;
    app.Init();
}

void loop() {
}
