#ifndef APPNETWORK_H_
#define APPNETWORK_H_

#include "Application.h"

class AppNetwork : public Application
{
public:
    AppNetwork() = default;
    ~AppNetwork() = default;
    bool Init() override;
    void Run() override;
private:
    bool SetupWiFi();
private:
    std::string m_ImagePath;
    EPDL::ImageHandle m_ImageHandle;
    bool m_ProcessImage = false;
};

#endif /*APPNETWORK_H_*/