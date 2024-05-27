#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include "Application.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
#include "AppFailsafe.h"
#include "AppDefault.h"
#include "Log.h"
#include "Clock.h"
#include "Config.h"

Application* Application::Create(std::string_view mode) {
    Log::Info("Starting application");
    
    if (mode == "Default"){
        return new AppDefault();
    }
    else if(mode == "Standalone") {
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
        Log::Fatal("Configuration wrong. Starting default app");
        return new AppDefault();
    }
}
