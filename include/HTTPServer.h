#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <map>
#include <PsychicHttp.h>
#include <string>
#include <vector>
#include <functional>

class HTTPServer {
public:
    HTTPServer() = default;
    ~HTTPServer() = default;
    void Init();
    void SetFilesToServe(const std::map<std::string, std::string>& files);
    void AddAPISetOpMode();
    void AddAPISetWiFiCred();
    void AddAPIUploadImg(std::function<void(std::string_view filePath)> callback = nullptr);
    void AddAPIls();
    void AddAPIrm();
    void AddAPImkdir();
private:
    PsychicHttpServer m_Server;
    bool status_AddAPIUploadImg = false;
};

#endif /*HTTPSERVER_H_*/
