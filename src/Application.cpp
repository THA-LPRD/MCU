#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include "Application.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
#include "AppFailsafe.h"
#include "AppConfig.h"
#include "Log.h"
#include "Clock.h"
#include "Config.h"

Application* Application::Create(std::string_view mode) {
    Log::Info("Starting application");
    std::string modestr = std::string(mode);
    Log::Info("App mode: %s", modestr);

    if (mode == "Config") {
        return new AppConfig();
    }
    else if (mode == "Standalone") {
        return new AppStandalone();
    }
    else if (mode == "Network") {
        return new AppNetwork();
    }
    else if (mode == "Server") {
        // TODO Implement AppServer
        // return new AppServer();
    }
    else {
        // TODO Implement AppFailsafe
        Log::Fatal("Invalid app mode. Starting Failsafe app ");
        return new AppFailsafe();
    }
}
