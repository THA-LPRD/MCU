#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "HTTPServer.h"

class Application
{
public:
    Application() {};
    virtual ~Application() = default;
    virtual bool Init() = 0;
    virtual void Run() = 0;
    static Application* Create(std::string_view mode);
protected:
    HTTPServer m_Server;
};

#endif /*APPLICATION_H_*/
