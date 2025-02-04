// #include "spdlog/spdlog.h"
// #include "spdlog/sinks/stdout_color_sinks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "GD_7IN5.h"
#include <Drivers/GPIO.h>
#include <Drivers/SPI.h>

GD_7IN5::GD_7IN5(int busyPin, int resetPin, int dcPin, int csPin, int sckPin, int mosiPin) :
        Driver(busyPin, resetPin, dcPin, csPin, sckPin, mosiPin),
        m_FrameBuffer(WIDTH, HEIGHT, COLOR_DEPTH),
        m_ColorPalette({
                               {0,   0,   0},   // Black
                               {255, 255, 255}, // White
                               {255, 0,   0}    // Red
                       }) {
    GPIO::SetMode(busyPin, GPIO::Mode::Input);
    GPIO::SetMode(resetPin, GPIO::Mode::Output);
    GPIO::SetMode(dcPin, GPIO::Mode::Output);
    GPIO::SetMode(csPin, GPIO::Mode::Output);
    GPIO::SetMode(sckPin, GPIO::Mode::Output);
    GPIO::SetMode(mosiPin, GPIO::Mode::Input);
    m_SPIController.SetFrequency(SPIFreq::MHZ_10);
    m_SPIController.SetPins(mosiPin, -1, sckPin, csPin);
    m_DriverInfo = {
            .Width = WIDTH,
            .Height = HEIGHT,
            .ColorDepth = COLOR_DEPTH,
            .PartialRefresh = true,
            .DriverName = "GooDisplay 7.5 inch 3-Color with Partial Refresh"
    };
}

void GD_7IN5::Initialize() {
    spdlog::debug("{} Initializing GD_7IN5 Display Driver", LOG_TAG);

    m_SPIController.Start(SPIDevice::SPI2);

    // Software reset
    Reset();

    SendCommand(0x01);
    SendData(0x07);
    SendData(0x07);
    SendData(0x3f);
    SendData(0x3f);

    SendCommand(0x06);			//Booster Soft Start 
	SendData(0x17);
	SendData(0x17);   
	SendData(0x28);		
	SendData(0x17);		
	
	PowerOn();

    WaitUntilReady();

    SendCommand(0X00);			//PANNEL SETTING
	SendData(0x0F);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

	SendCommand(0x61);			//resolution setting
	SendData(WIDTH/256); 
	SendData(WIDTH%256); 	
	SendData(HEIGHT/256);
	SendData(HEIGHT%256); 

	SendCommand(0X15);		
	SendData(0x00);		

	SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING
	SendData(0x11);  //0x10  --------------
	SendData(0x07);

	SendCommand(0X60);			//TCON SETTING
	SendData(0x22);
    spdlog::info("{} Initialized GD_7IN5 Display Driver", LOG_TAG);
}

void GD_7IN5::DrawImage(int handle, int x_offset, int y_offset) {
    spdlog::debug("{} Drawing image {} at ({}, {})", LOG_TAG, handle, x_offset, y_offset);
    if (PNGs.find(handle) == PNGs.end()) {
        spdlog::error("{} Image handle {} not found", LOG_TAG, handle);
        return;
    }
    auto filename = *PNGs[handle];
    // PNGDecoder::FileOps fileOps = PNGDecoder::LittleFSFileOps();
    PNGDecoder::FileOps fileOps = PNGDecoder::SDFileOps();
    m_PNGDecoder.Decode(filename, [this, x_offset, y_offset](int x, int y, const RGB &color) {
        uint8_t _color = this->m_ColorPalette.GetClosestColor(color);
        this->m_FrameBuffer.SetPixel(x + x_offset, y + y_offset, _color);
    }, fileOps, true);
}

void GD_7IN5::BeginFrame() {
    spdlog::debug("{} Begin frame", LOG_TAG);
    m_FrameBuffer.ClearColor(Color::White);
    spdlog::info("{} Cleared Framebuffer", LOG_TAG);
    Initialize();
}

void GD_7IN5::EndFrame() {
    spdlog::debug("{} End frame", LOG_TAG);
    Refresh();
    PowerOff();
    Sleep();
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void GD_7IN5::SwapBuffers() {
    spdlog::debug("{} Swap buffers", LOG_TAG);
    // One Framebuffer 48000 Bytes = 480 * 100 Bytes => 1 Bit per Pixel, Two Fraembuffers

    // BW Framebuffer
    StartDataTransmissionBW();
    for (uint16_t y = 0; y < HEIGHT; y++) {
        for (uint16_t x = 0; x < WIDTH / 8; x++) { 
            uint8_t data = 0;

            for (uint8_t k = 0; k < 8; k++) {
                uint8_t pixel = m_FrameBuffer.GetPixel(x * 8 + k, y);
                // Set Bit to 0 when Pixel is black (0)
                data |= (pixel << (7 - k));
            }
            SendData(data);
        }
    }

    // RW Framebuffer
    StartDataTransmissionRW();
    for (uint16_t y = 0; y < HEIGHT; y++) {
        for (uint16_t x = 0; x < WIDTH / 8; x++) {
            uint8_t data = 0;

            for (uint8_t k = 0; k < 8; k++) {
                uint8_t pixel = m_FrameBuffer.GetPixel(x * 8 + k, y);
                // Set Bit to 1 when Pixel is red (2)
                if (pixel == 2) {
                    data |= (1 << (7 - k));
                }
            }
            SendData(data);
        }
    }
}

void GD_7IN5::SendCommand(uint8_t command) {
    GPIO::Write(m_DCPin, 0);
    m_SPIController.Write(command);
}

void GD_7IN5::SendData(uint8_t data) {
    GPIO::Write(m_DCPin, 1);
    m_SPIController.Write(data);
}

void GD_7IN5::WaitUntilReady() {
    //LOW: busy, HIGH: idle
    spdlog::debug("{} Waiting for display", LOG_TAG);
    while (GPIO::Read(m_BusyPin) == 0) {
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    spdlog::debug("{} Display ready", LOG_TAG);
}

void GD_7IN5::Reset() {
    GPIO::Write(m_ResetPin, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    GPIO::Write(m_ResetPin, 0);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    GPIO::Write(m_ResetPin, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    WaitUntilReady();
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void GD_7IN5::PowerOff() {
    SendCommand(0X50);  //VCOM AND DATA INTERVAL SETTING			
	SendData(0xf7); //WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7	
	SendCommand(0X02);  	//power off
}

void GD_7IN5::PowerOn() {
    SendCommand(0x04);
}

void GD_7IN5::Sleep() {
    WaitUntilReady(); //waiting for the electronic paper IC to release the idle signal
    vTaskDelay(1 / portTICK_PERIOD_MS); //!!!The delay here is necessary, 200uS at least!!! 
    SendCommand(0x07); //deep sleep
    SendData(0xA5);
}

void GD_7IN5::StartDataTransmissionBW() {
    SendCommand(0x10);
}

void GD_7IN5::StartDataTransmissionRW() {
    SendCommand(0x13);
}

void GD_7IN5::Refresh() {
    SendCommand(0x12);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    WaitUntilReady();
}
