#ifndef LPRD_MCU_APPSERVER_H
#define LPRD_MCU_APPSERVER_H

#include "Application.h"

class AppServer final : public Application {
public:
    explicit AppServer() = default;
    ~AppServer() override = default;
    uint64_t Run() override { return 0; }
protected:
    bool InitImpl() override { return true; };
    HTTPServer m_Server;
};

#endif //LPRD_MCU_APPSERVER_H
