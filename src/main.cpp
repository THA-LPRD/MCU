#include <Arduino.h>
#include "Log.h"
#include "Clock.h"

void FuncLog(char* msg) {
    time_t time = Clock::GetTime();
    struct tm* timeinfo = localtime(&time);
    char time_str[18];
    strftime(time_str, 18, "%y-%m-%d %H:%M:%S", timeinfo);
    String log_msg = "[" + String(time_str) + "] - " + String(msg);
    Serial.println(log_msg.c_str());
}

void setup() {
    Serial.begin(115200);
    Clock::SetTime(2024, 1, 1, 0, 0, 0, -1);
    Log::SetLogFunction(FuncLog);
}

void loop() {
    static int i = 0;
    int j = i++ % 5;
    switch (j) {
    case 0:
        Log::Debug("Debug message from Loop(%d)", i);
        break;
    case 1:
        Log::Info("Info message from Loop(%d)", i);
        break;
    case 2:
        Log::Warn("Warning message from Loop(%d)", i);
        break;
    case 3:
        Log::Error("Error message from Loop(%d)", i);
        break;
    case 4:
        Log::Fatal("Fatal message from Loop(%d)", i);
        break;
    }
    delay(1000);
}
