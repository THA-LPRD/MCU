#ifndef LPRD_MCU_APPSERVER_H
#define LPRD_MCU_APPSERVER_H

#include "Application.h"
#include "HttpClient.h"

class AppServer : public Application {
public:
    AppServer() = default;
    ~AppServer() override;
    bool InitImpl() override;
    uint64_t Run() override;
protected:
    uint64_t m_SleepTime = 0;
private:
    std::string MacToHex(uint8_t* mac);
    std::string ServerURL();
    bool CheckIfRegistered(uint8_t* mac);
    bool RegisterOnServer(uint8_t* mac);
    std::string FetchConfig(uint8_t* mac);
    bool FetchImg(std::string_view imageURLPath);
    bool DrawImg();
private:
    HTTPServer m_Server = HTTPServer("/api/v2");
    HttpClient m_HttpClient = HttpClient();
};

#endif //LPRD_MCU_APPSERVER_H
