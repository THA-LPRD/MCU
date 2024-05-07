#ifndef APPSTANDALONE_H_
#define APPSTANDALONE_H_

#include "Application.h"
#include <DNSServer.h>

class AppStandalone : public Application
{
public:
    AppStandalone() = default;
    ~AppStandalone() = default;
    bool Init() override;
    void Run() override { m_DNSServer.processNextRequest(); }
private:
    bool SetupWiFi();
private:
    DNSServer m_DNSServer;
};

#endif /*APPSTANDALONE_H_*/