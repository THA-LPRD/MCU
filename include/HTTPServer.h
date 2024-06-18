#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include <map>
#include <PsychicHttp.h>
#include <string>
#include <vector>
#include <functional>
#include <PsychicHttpsServer.h>

class HTTPServer {
public:
    HTTPServer() = default;
    ~HTTPServer();
    bool Init(bool https = false);
    void SetFilesToServe(const std::map<std::string, std::string> &files);
    void AddAPISetOpMode();
    void AddAPISetWiFiCred();
    void AddAPISetServerURL();
    void AddAPIUploadImg(const std::function<void(std::string_view filePath)>& callback = nullptr);
    void AddAPIGetDisplayModule();
    void AddAPISetDisplayModule();
    void AddAPIGetOpMode();
    void AddAPIGetDisplayWidth();
    void AddAPIGetDisplayHeight();
    void AddAPIls();
    void AddAPIrm();
    void AddAPImkdir();
    void AddAPIRestart();
    void AddAPISetLogLevel();
    void AddAPIGetLogLevel();
    void EnableHTTPAuth(std::string_view username, std::string_view password);
    void AddAPISetHTTPAuth();
    void AddAPISetHTTPS();
    void AddAPIGetHTTPS();
    void AddAPIUploadHTTPSKey();
    void AddAPIUploadHTTPSCert();
private:
    bool InitHTTPS();
    bool InitHTTP();
private:
    PsychicHttpServer m_HTTPServer;
    PsychicHttpsServer m_HTPPSServer;
    PsychicHttpServer* m_MainServer = nullptr;
    bool status_AddAPIUploadImg = false;
    std::vector<PsychicHandler*> m_Handlers;
    bool m_isHTTPS = false;
};

#endif /*HTTPSERVER_H_*/
