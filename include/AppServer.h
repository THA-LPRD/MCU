#ifndef APPSERVER_H_
#define APPSERVER_H_

#include "Application.h"
#include <DNSServer.h>
#include <HTTPClient.h>
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
    bool CheckRegistered(uint8_t *mac);
    bool RegisterDisplay(uint8_t *mac);
    String GetNewConfig(uint8_t *mac);
    std::string GetNewImage(String imageURLPath);
    void DisplayImage(std::string path);

private:
    DNSServer m_DNSServer;
    std::string m_ImagePath;
    EPDL::ImageHandle m_ImageHandle;
    bool m_ProcessImage = false;
};

#endif /*APPSERVER_H_*/