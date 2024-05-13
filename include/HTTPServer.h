#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <map>
#include <PsychicHttp.h>
#include <string>
#include <vector>

class HTTPServer {
public:
    HTTPServer() = default;
    ~HTTPServer() = default;
    void Init(int* RenderIMG);
private:
    PsychicHttpServer m_Server;
    std::map<std::string, std::vector<uint8_t>> uploadBuffers;
};

#endif /*HTTPSERVER_H_*/
