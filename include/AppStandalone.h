#ifndef APPSTANDALONE_H_
#define APPSTANDALONE_H_

#include "Application.h"
#include <DNSServer.h>
#include <EPDL.h>

class AppStandalone : public Application {
public:
    AppStandalone() = default;
    ~AppStandalone() = default;
    bool Init() override;
    void Run() override;
private:
    bool SetupWiFi();
private:
    DNSServer m_DNSServer;
    std::string m_ImagePath;
    EPDL::ImageHandle m_ImageHandle;
    bool m_ProcessImage = false;
};

#endif /*APPSTANDALONE_H_*/