#include "IT8951E.h"
#include <Log.h>
#include <GPIO.h>
#include <MCU.h>
#include <cstring>

namespace EPDL
{
    IT8951E::IT8951E() {
        Log::Debug("[EPDL] Initializing IT8951E controller board driver");

        m_SPIController = MCU::SPI::Create(static_cast<MCU::SPIDevice>(EPDL::SPI::SPIDevice),
                                           EPDL::Pin::MOSI,
                                           EPDL::Pin::DC,
                                           EPDL::Pin::SCK,
                                           -1,
                                           false);

        Reset();

        m_DeviceInfo = GetDeviceInfo();
        Log::Info("[EPDL] IT8951E controller board driver initialized");
        Log::Info("[EPDL] Panel size: %d x %d", m_DeviceInfo.PanelWidth, m_DeviceInfo.PanelHeight);
        Log::Info("[EPDL] FW Version: %s", (uint8_t*) m_DeviceInfo.FWVersion);
        Log::Info("[EPDL] LUT Version: %s", (uint8_t*) m_DeviceInfo.LUTVersion);
        EnableI80PackedMode();

        Log::Debug("[EPDL] IT8951E controller board driver initialized");
    }

    IT8951E::~IT8951E() {
        Log::Debug("[EPDL] Deinitializing IT8951E controller board driver");
        delete m_SPIController;
        m_SPIController = nullptr;
        Log::Info("[EPDL] IT8951E controller board driver deinitialized");
    }

    void IT8951E::SendData(uint16_t* data, uint16_t length) {
        Log::Trace("[EPDL] Sending data to IT8951E controller board");

        if (length == 0) return;
        if (data == nullptr) {
            Log::Error("[EPDL] No data provided to send");
            return;
        }

        uint8_t buffer[2];

        WaitUntilReady();
        MCU::GPIO::Write(EPDL::Pin::CS, 0);
        // Preamble: 0x0000
        buffer[0] = 0x00;
        buffer[1] = 0x00;
        m_SPIController->Write(buffer, 2);
        for (uint16_t i = 0; i < length; i++) {
            WaitUntilReady();
            buffer[0] = data[i] >> 8;
            buffer[1] = data[i] & 0xFF;
            m_SPIController->Write(buffer, 2);
        }
        MCU::GPIO::Write(EPDL::Pin::CS, 1);
    }

    void IT8951E::SendCommand(uint16_t cmd, uint16_t* args, uint16_t argCount) {
        Log::Trace("[EPDL] Sending command to IT8951E controller board");
        uint8_t buffer[2];

        WaitUntilReady();
        MCU::GPIO::Write(EPDL::Pin::CS, 0);
        // Preamble: 0x6000
        buffer[0] = 0x60;
        buffer[1] = 0x00;
        m_SPIController->Write(buffer, 2);

        WaitUntilReady();
        buffer[0] = cmd >> 8;
        buffer[1] = cmd & 0xFF;
        m_SPIController->Write(buffer, 2);
        MCU::GPIO::Write(EPDL::Pin::CS, 1);

        SendData(args, argCount);
    }

    std::vector<uint16_t> IT8951E::Read(size_t length) {
        Log::Trace("[EPDL] Reading data from IT8951E controller board");
        std::vector<uint16_t> data;
        std::vector<uint8_t> buffer(2);

        WaitUntilReady();
        MCU::GPIO::Write(EPDL::Pin::CS, 0);
        // Preamble: 0x1000
        buffer[0] = 0x10;
        buffer[1] = 0x00;
        m_SPIController->Write(buffer.data(), 2);
        WaitUntilReady();
        m_SPIController->Read(2); // Dummy read
        for (size_t i = 0; i < length; i++) {
            WaitUntilReady();
            buffer = m_SPIController->Read(2);
            data.push_back(buffer[0] << 8 | buffer[1]);
        }
        MCU::GPIO::Write(EPDL::Pin::CS, 1);
        return data;
    }

    void IT8951E::EnableI80PackedMode() {
        Log::Debug("[EPDL] Enabling I80 packed mode");
        uint16_t args[] = {
                0x0004, // Register I80CPCR
                0x0001};
        SendCommand(0x0011, args, 2);
    }

    void IT8951E::WaitUntilReady() {
        while (MCU::GPIO::Read(EPDL::Pin::BUSY) == 0) {
            MCU::Sleep(20);
        }
    }

    void IT8951E::Reset() {
        MCU::GPIO::Write(EPDL::Pin::RST, 0);
        MCU::Sleep(1000);
        MCU::GPIO::Write(EPDL::Pin::RST, 1);
        MCU::Sleep(20);
    }

    IT8951E::DeviceInfo IT8951E::GetDeviceInfo() {
        Log::Debug("[EPDL] Getting device info");
        DeviceInfo info = {0};
        SendCommand(0x0302);
        std::vector<uint16_t> data = Read(sizeof(DeviceInfo) / 2);
        std::memcpy(&info, data.data(), sizeof(DeviceInfo));
        return info;
    }

    void IT8951E::StartTransmission() {
        WaitUntilDisplayReady();
        uint16_t args[5];
        args[0] = 0x0011, // TCON Register Write
        args[1] = 0x0210, // Addr: Lisar Register
        args[2] = m_DeviceInfo.ImgBufAddrH;
        SendCommand(0x0010, args, 3);

        args[0] = 0x0011; // TCON Register Write
        args[1] = 0x0208; // Addr: Lisar Register
        args[2] = m_DeviceInfo.ImgBufAddrL;
        SendCommand(0x0010, args, 3);

        args[0] = m_LoadImageInfo.Endianness << 8 | m_LoadImageInfo.BitsPerPixel << 4 | m_LoadImageInfo.Rotate;
        args[1] = 0;
        args[2] = 0;
        args[3] = m_DeviceInfo.PanelWidth;
        args[4] = m_DeviceInfo.PanelHeight;
        SendCommand(0x0021, args, 5); // TCON Load Image Area
    }

    void IT8951E::EndTransmission() {
        WaitUntilDisplayReady();
        SendCommand(0x0022); // TCON Load Image End
        uint16_t args[] = {
                0x0000,
                0x0000,
                m_DeviceInfo.PanelWidth,
                m_DeviceInfo.PanelHeight,
                0x0002}; // Driver Mode
        SendCommand(0x0034, args, 5); // Display Area
    }

    void IT8951E::WaitUntilDisplayReady() {
        auto busy = [&]() -> bool {
            uint16_t args[] = {0x1224};
            SendCommand(0x0010, args, 1);
            return Read(1)[0] == 0;
        };
        while (!busy()) {
            MCU::Sleep(20);
        }
    }
} // namespace EPDL
