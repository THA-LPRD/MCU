#ifndef APPDEFAULT_H_
#define APPDEFAULT_H_

#include "Application.h"

class AppDefault : public Application
{
public:
    AppDefault() = default;
    ~AppDefault() = default;
    bool Init() override;
    void Run() override {};
private:
    bool SetupWiFi();
private:
    
};

#endif /*APPDEFAULT_H_*/