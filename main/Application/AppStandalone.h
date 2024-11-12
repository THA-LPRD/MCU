#ifndef LPRD_MCU_APPSTANDALONE_H
#define LPRD_MCU_APPSTANDALONE_H

#include "AppHost.h"

class AppStandalone final : public AppHost {
public:
    AppStandalone() = default;
    ~AppStandalone() final;
    bool InitImpl() final;
    uint64_t Run() final;
};

#endif //LPRD_MCU_APPSTANDALONE_H
