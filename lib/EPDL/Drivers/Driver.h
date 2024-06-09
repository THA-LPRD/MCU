#ifndef DRIVER_H_
#define DRIVER_H_

#include <cstddef>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include "EPDL.h"
#include <SPI.h>

namespace EPDL
{
    class Driver {
    public:
        Driver() = default;
        virtual ~Driver() = default;
        virtual ImageHandle CreateImage(std::unique_ptr<ImageData> data) {
            ImageHandle handle = index++;
            m_ImageData[handle] = std::move(data);
            return handle;
        }

        inline virtual void DeleteImage(ImageHandle handle) { m_ImageData.erase(handle); }
        virtual void DrawImage(ImageHandle handle, int x = 0, int y = 0) = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void SwapBuffers() = 0;
        virtual uint16_t GetWidth() const = 0;
        virtual uint16_t GetHeight() const = 0;
    protected:
        MCU::SPI* m_SPIController;
        std::unordered_map<ImageHandle, std::unique_ptr<ImageData>> m_ImageData;
        int index = 0;
    };
} // namespace EPDL

#endif /*DRIVER_H_*/
