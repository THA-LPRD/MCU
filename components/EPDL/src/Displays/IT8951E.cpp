#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "IT8951E.h"
#include <Drivers/GPIO.h>
#include <Drivers/SPI.h>
#include <cstring>

IT8951E::IT8951E(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin) :
        Driver(busyPin, resetPin, dcPin, csPin, sckPin, mosiPin) {
    GPIO::SetMode(busyPin, GPIO::Mode::Input);
    GPIO::SetMode(resetPin, GPIO::Mode::Output);
    GPIO::SetMode(dcPin, GPIO::Mode::Input);
    GPIO::SetMode(csPin, GPIO::Mode::Output);
    GPIO::SetMode(sckPin, GPIO::Mode::Output);
    GPIO::SetMode(mosiPin, GPIO::Mode::Input);
    m_SPIController.SetFrequency(SPIFreq::MHZ_10);
    m_SPIController.SetPins(mosiPin, dcPin, sckPin, -1);
}

void IT8951E::Initialize() {
    spdlog::debug("{} Initializing IT8951E controller board driver", LOG_TAG);

    m_SPIController.Start(SPIDevice::SPI2);

    Reset();

    m_DeviceInfo = GetDeviceInfo();
    spdlog::info("{} Panel size: {} x {}", LOG_TAG, m_DeviceInfo.PanelWidth, m_DeviceInfo.PanelHeight);
    spdlog::info("{} FW Version: {}", LOG_TAG, (char*) m_DeviceInfo.FWVersion);
    spdlog::info("{} LUT Version: {}", LOG_TAG, (char*) m_DeviceInfo.LUTVersion);

    EnableI80PackedMode();

    spdlog::info("{} IT8951E controller board driver initialized", LOG_TAG);
}

void IT8951E::SendData(const uint16_t* data, uint16_t length) {
    if (length == 0) return;
    if (data == nullptr) {
        spdlog::error("{} No data provided to send", LOG_TAG);
        return;
    }

    uint8_t buffer[2];

    WaitUntilReady();
    GPIO::Write(m_CSPin, 0);
    // Preamble: 0x0000
    buffer[0] = 0x00;
    buffer[1] = 0x00;
    m_SPIController.Write(buffer, 2);
    for (uint16_t i = 0; i < length; i++) {
        WaitUntilReady();
        buffer[0] = data[i] >> 8;
        buffer[1] = data[i] & 0xFF;
        m_SPIController.Write(buffer, 2);
    }
    GPIO::Write(m_CSPin, 1);
}

void IT8951E::SendCommand(uint16_t cmd, uint16_t* args, uint16_t argCount) {
    uint8_t buffer[2];

    WaitUntilReady();
    GPIO::Write(m_CSPin, 0);
    // Preamble: 0x6000
    buffer[0] = 0x60;
    buffer[1] = 0x00;
    m_SPIController.Write(buffer, 2);

    WaitUntilReady();
    buffer[0] = cmd >> 8;
    buffer[1] = cmd & 0xFF;
    m_SPIController.Write(buffer, 2);
    GPIO::Write(m_CSPin, 1);

    SendData(args, argCount);
}

std::vector<uint16_t> IT8951E::Read(size_t length) {
    std::vector<uint16_t> data;
    std::vector<uint8_t> buffer(2);

    WaitUntilReady();
    GPIO::Write(m_CSPin, 0);
    // Preamble: 0x1000
    buffer[0] = 0x10;
    buffer[1] = 0x00;
    m_SPIController.Write(buffer.data(), 2);
    WaitUntilReady();
    m_SPIController.Read(2); // Dummy read
    for (size_t i = 0; i < length; i++) {
        WaitUntilReady();
        buffer = m_SPIController.Read(2);
        data.push_back(buffer[0] << 8 | buffer[1]);
    }
    GPIO::Write(m_CSPin, 1);
    return data;
}

void IT8951E::EnableI80PackedMode() {
    uint16_t args[] = {
            0x0004, // Register I80CPCR
            0x0001};
    SendCommand(0x0011, args, 2);
}

void IT8951E::WaitUntilReady() {
    // LOW: idle, HIGH: busy
    while (GPIO::Read(m_BusyPin) == 0) {
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void IT8951E::Reset() {
    GPIO::Write(m_ResetPin, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    GPIO::Write(m_ResetPin, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

IT8951E::DeviceInfo IT8951E::GetDeviceInfo() {
    spdlog::debug("{} Getting device info", LOG_TAG);
    DeviceInfo info = {};
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
    SendCommand(0x0034, args, 5); // second refresh to avoid ghosting
}

void IT8951E::WaitUntilDisplayReady() {
    auto busy = [&]() -> bool {
        uint16_t args[] = {0x1224};
        SendCommand(0x0010, args, 1);
        return Read(1)[0] == 0;
    };
    while (!busy()) {
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void IT8951E::Sleep() {
    SendCommand(0x0003);
}