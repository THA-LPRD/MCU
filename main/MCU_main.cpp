#include "Log.h"
#include "Arduino.h"

extern "C" void app_main(void)
{
    initArduino();

    Log::SetLogLevel(Log::Level::DEBUG);
    for (int i = 0; i < 10; i++)
    {
        Log::Trace("Hello, world! %d", i);
        Log::Debug("Hello, world! %d", i);
        Log::Info("Hello, world! %d", i);
        Log::Warning("Hello, world! %d", i);
        Log::Error("Hello, world! %d", i);
        Log::Fatal("Hello, world! %d", i);
    }

}
