#ifndef APPFAILSAFE_H_
#define APPFAILSAFE_H_



#include "Application.h"


class AppFailsafe : public Application
{
public:
    AppFailsafe() = default;
    ~AppFailsafe() = default;
    bool Init() override;
    void Run() override {};
private:
    bool SetupWiFi();
private:
    
};

#endif /*APPFAILSAFE_H_*/