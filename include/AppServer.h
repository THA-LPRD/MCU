#ifndef APPSERVER_H_
#define APPSERVER_H_

#include "Application.h"
#include <DNSServer.h>
#include <EPDL.h>

class AppServer : public Application {
public:
    AppServer() = default;
    ~AppServer() = default;
    bool Init() override;
    void Run() override;
private:
    bool SetupWiFi();
    bool DisconnectWiFi();
private:
    DNSServer m_DNSServer;
    std::string m_ImagePath;
    EPDL::ImageHandle m_ImageHandle;
    bool m_ProcessImage = false;
};

#endif /*APPSERVER_H_*/