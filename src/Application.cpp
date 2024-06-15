#include <string>
#include "Application.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
#include "AppServer.h"
#include "AppDefault.h"
#include "Log.h"
#include "MCU.h"

Application* Application::Create(std::string_view mode) {
    Log::Info("Starting application");
    Log::Info("App mode: %s", mode.data());

    Application* app = nullptr;

    if (mode == "Standalone") {
        app = new AppStandalone();
    }
    else if (mode == "Network") {
        app = new AppNetwork();
    }
    else if (mode == "Server") {
        // TODO Implement AppServer
        app = new AppServer();
    }
    else {
        app = new AppDefault();
    }

    if (app == nullptr) {
        Log::Fatal("Failed to create application");
        MCU::Restart();
    }

    return app;
}
