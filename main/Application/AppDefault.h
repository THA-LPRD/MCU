#ifndef LPRD_MCU_APPDEFAULT_H
#define LPRD_MCU_APPDEFAULT_H

#include "Application.h"

class AppDefault : public Application
{
public:
    AppDefault(std::shared_ptr<spdlog::logger> logger);
    ~AppDefault() = default;
    bool Init() override;
    void Run() override {};
private:
    bool SetupWiFi();
};

#endif //LPRD_MCU_APPDEFAULT_H
