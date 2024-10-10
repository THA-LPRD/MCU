#include "AppDefault.h"

AppDefault::AppDefault(std::shared_ptr<spdlog::logger> logger){}

bool AppDefault::Init() {
    m_Logger->info("Initializing AppDefault");
    return true;
}

bool AppDefault::SetupWiFi() {
    return true;
}

