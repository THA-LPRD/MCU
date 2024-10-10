#include "AppStandalone.h"

AppStandalone::AppStandalone(std::shared_ptr<spdlog::logger> logger){}

AppStandalone::~AppStandalone() = default;

bool AppStandalone::Init() {
    m_Logger->info("Initializing Standalone Application");
    return true;
}

void AppStandalone::Run() {
    m_Logger->info("Running Standalone Application");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

bool AppStandalone::SetupWiFi() {
    return true;
}

void AppStandalone::DestroyWiFi() {
}