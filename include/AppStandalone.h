#ifndef APPSTANDALONE_H_
#define APPSTANDALONE_H_

#include "Application.h"
#include <DNSServer.h>
#include <EPDL.h>

class AppStandalone : public Application {
public:
    AppStandalone() = default;
    ~AppStandalone() override;
    bool Init() override;
    void Run() override;
private:
    bool SetupWiFi();
    void DestroyWiFi();
private:
    DNSServer m_DNSServer;
    std::string m_ImagePath;
    EPDL::ImageHandle m_ImageHandle = -1;
    bool m_ProcessImage = false;
};

#endif /*APPSTANDALONE_H_*/