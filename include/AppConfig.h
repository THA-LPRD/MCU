#ifndef APPCONFIG_H_
#define APPCONFIG_H_



#include "Application.h"


class AppConfig : public Application
{
public:
    AppConfig() = default;
    ~AppConfig() = default;
    bool Init() override;
    void Run() override {};
private:
    bool SetupWiFi();
private:
    
};

#endif /*APPCONFIG_H_*/