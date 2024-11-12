#ifndef LPRD_MCU_APPNETWORK_H
#define LPRD_MCU_APPNETWORK_H

#include "AppHost.h"

class AppNetwork final : public AppHost {
public:
    AppNetwork() = default;
    ~AppNetwork() final;
    bool InitImpl() final;
    uint64_t Run() final;
};

#endif //LPRD_MCU_APPNETWORK_H
