#ifndef LPRD_MCU_APPSTANDALONE_H
#define LPRD_MCU_APPSTANDALONE_H

#include "Application.h"

class AppStandalone : public Application {
public:
    AppStandalone(std::shared_ptr<spdlog::logger> logger);
    ~AppStandalone() override;
    bool Init() override;
    void Run() override;
private:
    bool SetupWiFi();
    void DestroyWiFi();
};

#endif //LPRD_MCU_APPSTANDALONE_H
