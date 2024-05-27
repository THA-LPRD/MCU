#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

class HTTPServer {
public:
    HTTPServer();
    ~HTTPServer() = default;
    void Init();
private:
    AsyncWebServer m_Server;
};

#endif /*HTTPSERVER_H_*/
