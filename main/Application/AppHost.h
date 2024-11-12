#ifndef LPRD_MCU_APPHOST_H
#define LPRD_MCU_APPHOST_H

#include "Application.h"
#include <memory>
#include <spdlog/spdlog.h>
#include "freertos/event_groups.h"
#include "../HTTPServer/HTTPServer.h"

struct test_pre_server_deconstructor {
    test_pre_server_deconstructor() {
        spdlog::info("Pre server deconstructor");
    }
    ~test_pre_server_deconstructor() {
        spdlog::info("pre server deconstructor");
    }
};

struct test_post_server_deconstructor {
    test_post_server_deconstructor() {
        spdlog::info("post server deconstructor");
    }
    ~test_post_server_deconstructor() {
        spdlog::info("Post server deconstructor");
    }
};

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
    void InitServerStandalone();
    void InitServerNetwork();
    void InitServerServer();
    void InitServerHTTP();
private:
    // Deconstructor called after server is destroyed
    test_pre_server_deconstructor m_PreServerDeconstructor = test_pre_server_deconstructor();
    HTTPServer m_Server = HTTPServer("/api/v2");
    // Deconstructor called before server is destroyed
    test_post_server_deconstructor m_PostServerDeconstructor = test_post_server_deconstructor();
};

#endif //LPRD_MCU_APPHOST_H
