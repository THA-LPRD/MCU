#ifndef LPRD_MCU_APPSERVER_H
#define LPRD_MCU_APPSERVER_H
/* */
#include "Application.h"

// Eduroam Fix
#include "Drivers/Eduroam.h"

class AppServer : public Application {
public:
    AppServer() = default;
    ~AppServer() override;
    bool InitImpl() override;
    uint64_t Run() override;
protected:
    uint64_t m_SleepTime = 0;
private:
    bool CheckIfRegistered(uint8_t* mac);
    bool RegisterOnServer(uint8_t* mac);
    String FetchConfig(uint8_t* mac);
    bool FetchImg(const std::string& imageURLPath);
    bool DrawImg();
private:
    HTTPServer m_Server = HTTPServer("/api/v2");
    // Eduroam Fix
    Eduroam m_Eduroam;
};
/**/
#endif //LPRD_MCU_APPSERVER_H
