#include <memory>
#include <DEV_Config.h>
#include "EPDL.h"
#include "DisplayDrivers/Driver.h"
#include "DisplayDrivers/WS_7IN3G.h"
#include "Log.h"

namespace EPDL
{
    namespace
    { // Private members
        static std::unique_ptr<Driver> m_Driver = nullptr;
    } // namespace

    int Init() {
        DEV_Module_Init();
        return 0;
    }

    void Terminate(){
        return;
    }

    void LoadDriver(std::string_view type) {
        if (type == "WS_7IN3G") {
            m_Driver = std::make_unique<WS_7IN3G>();
        }
        else {
            Log::Warning("Unknown display driver: %s", type.data());
            m_Driver = nullptr;
        }
    }

    ImageHandle CreateImage(std::unique_ptr<ImageData> data){
        if (m_Driver == nullptr) {
            return -1;
        }
        return m_Driver->CreateImage(std::move(data));
    }

    void DeleteImage(ImageHandle handle){
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->DeleteImage(handle);
    }

    void DrawImage(ImageHandle handle, uint16_t x, uint16_t y){
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->DrawImage(handle, x, y);
    }

    void BeginFrame(){
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->BeginFrame();
    }

    void EndFrame(){
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->EndFrame();
    }

    void SwapBuffers(){
        if (m_Driver == nullptr) {
            return;
        }
        m_Driver->SwapBuffers();
    }

} // namespace EPDL