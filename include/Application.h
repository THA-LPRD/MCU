#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "HTTPServer.h"

class Application
{
public:
    enum class Mode
    {
        STANDALONE,
        NETWORK,
        SERVER,
    };
    Application() = default;
    ~Application() = default;
    void Init();
private:
    inline void SetMode(Mode mode) { m_Mode = mode; }
    bool SetupWiFi();
    void SetMode(std::string mode);
private:
    Mode m_Mode;
    HTTPServer m_Server;
};

#endif /*APPLICATION_H_*/
