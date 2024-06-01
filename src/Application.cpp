#include <string>
#include "Application.h"
#include "AppStandalone.h"
#include "AppNetwork.h"
#include "AppDefault.h"
#include "Log.h"

Application* Application::Create(std::string_view mode) {
    Log::Info("Starting application");
    Log::Info("App mode: %s", mode.data());

    if (mode == "Standalone") {
        return new AppStandalone();
    }
    else if (mode == "Network") {
        return new AppNetwork();
    }
    else if (mode == "Server") {
        // TODO Implement AppServer
        // return new AppServer();
    }
    return new AppDefault();

}
