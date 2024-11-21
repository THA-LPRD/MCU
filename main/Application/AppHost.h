#ifndef LPRD_MCU_APPHOST_H
#define LPRD_MCU_APPHOST_H

#include "Application.h"
#include <memory>
#include <spdlog/spdlog.h>
#include "freertos/event_groups.h"
#include "../HTTPServer/HTTPServer.h"

class AppHost : public Application {
public:
    AppHost() = default;
    ~AppHost() override;
    bool InitImpl() override = 0;
    uint64_t Run() override = 0;
protected:
    bool InitServer();
protected:
    uint64_t m_SleepTime = 0;
private:
    void InitServerCore();
    void InitServerHTTP();
private:
    HTTPServer m_Server = HTTPServer("/api/v2");
};

#endif //LPRD_MCU_APPHOST_H
