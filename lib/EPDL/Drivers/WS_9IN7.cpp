#include "WS_9IN7.h"
#include "Log.h"
#include <MCU.h>
#include <GPIO.h>
#include <SPI.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "pic.h"


namespace EPDL
{
    uint8_t* gpFrameBuf;
    uint32_t gulImgBufAddr;

    WS_9IN7::WS_9IN7() : m_FrameBuffer(400, 400, 4/*m_Width, m_Height, m_PixelSize*/) {
        Log::Debug("[EPDL] Initializing WS_9IN7 display driver");

        m_SPIController = MCU::SPI::Create(static_cast<MCU::SPIDevice>(EPDL::SPI::SPIDevice),
                                           EPDL::Pin::MOSI,
                                           EPDL::Pin::DC,
                                           EPDL::Pin::SCK,
                                           -1,
                                           false);

        Reset();

        IT8951DevInfo gstI80DevInfo; // TODO: Fix this

        // Init
        GetIT8951SystemInfo(&gstI80DevInfo);

        //Set to Enable I80 Packed mode
        IT8951WriteReg(WS_9IN7::I80::Cpcr, 0x0001);

        Log::Debug("[EPDL] WS_9IN7 display driver initialized");
    }

    void WS_9IN7::GetIT8951SystemInfo(void* pBuf) {
        uint16_t* pusWord = (uint16_t*) pBuf;
        IT8951DevInfo* pstDevInfo;

        //Send I80 CMD
        LCDWriteCmdCode(I80Cmd::DevInfo);

        //Burst Read Request for SPI interface only
        LCDReadNData(pusWord, sizeof(IT8951DevInfo) / 2);  //Polling HRDY for each words(2-bytes) if possible

        //Show Device information of IT8951
        pstDevInfo = (IT8951DevInfo*) pBuf;
        Log::Debug("Panel size: %d x %d", pstDevInfo->usPanelW, pstDevInfo->usPanelH);
        Log::Debug("FW Version: %s\r\n", (uint8_t*) pstDevInfo->usFWVersion);
        Log::Debug("LUT Version: %s\r\n", (uint8_t*) pstDevInfo->usLUTVersion);

        gulImgBufAddr = pstDevInfo->usImgBufAddrL | ((uint32_t) pstDevInfo->usImgBufAddrH << 16);
    }

    void WS_9IN7::LCDWriteCmdCode(uint16_t usCmdCode) {
        //Set Preamble for Write Command
        uint8_t wPreamble[2] = {0x60, 0x00};

        WaitUntilReady();
        MCU::GPIO::Write(EPDL::Pin::CS, 0);
        m_SPIController->Write(wPreamble, 2);
        WaitUntilReady();
        uint8_t data[2] = {0};
        data[0] = usCmdCode >> 8;
        data[1] = usCmdCode;
        m_SPIController->Write(data, 2);

        MCU::GPIO::Write(EPDL::Pin::CS, 1);
    }

    void WS_9IN7::LCDWriteData(uint16_t usData) {
        //Set Preamble for Write Data
        uint8_t wPreamble[2] = {0x00, 0x00};

        WaitUntilReady();
        MCU::GPIO::Write(EPDL::Pin::CS, 0);
        m_SPIController->Write(wPreamble, 2);
        WaitUntilReady();
        uint8_t data[2] = {0};
        data[0] = usData >> 8;
        data[1] = usData;
        m_SPIController->Write(data, 2);

        MCU::GPIO::Write(EPDL::Pin::CS, 1);
    }

    void WS_9IN7::LCDSendCmdArg(uint16_t usCmdCode, uint16_t* pArg, uint16_t usNumArg) {
        uint16_t i;
        //Send Cmd code
        LCDWriteCmdCode(usCmdCode);
        //Send Data
        for (i = 0; i < usNumArg; i++) {
            LCDWriteData(pArg[i]);
        }
    }

    void WS_9IN7::LCDReadNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt) {
        uint32_t i;
        uint8_t wPreamble[2] = {0x10, 0x00};

        WaitUntilReady();
        MCU::GPIO::Write(EPDL::Pin::CS, 0);
        m_SPIController->Write(wPreamble, 2);
        WaitUntilReady();
        m_SPIController->Read(2);
        WaitUntilReady();
        std::vector<uint8_t> temp = m_SPIController->Read(ulSizeWordCnt * 2);
        MCU::GPIO::Write(EPDL::Pin::CS, 1);

        for (i = 0; i < ulSizeWordCnt; i++) {
            pwBuf[i] = temp[i * 2] << 8;
            pwBuf[i] |= temp[i * 2 + 1];
        }
    }

    uint16_t WS_9IN7::LCDReadData() {
        uint16_t wRData;
        uint8_t wPreamble[2] = {0x10, 0x00};

        WaitUntilReady();

        MCU::GPIO::Write(EPDL::Pin::CS, 0);

        m_SPIController->Write(wPreamble, 2);
        WaitUntilReady();
        m_SPIController->Read(2);
        WaitUntilReady();
        std::vector<uint8_t> temp = m_SPIController->Read(2);

        MCU::GPIO::Write(EPDL::Pin::CS, 0);

        wRData = temp[0] << 8;
        wRData |= temp[1];

        return wRData;
    }

/*
    void WS9IN3::EPD_Clear(uint8_t Color) {
        //memset(gpFrameBuf, Color, gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH);
    }
*/
    void WS_9IN7::IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue) {
        //Send Cmd , Register Address and Write Value
        LCDWriteCmdCode(WS_9IN7::IT8951::TconRegWr);
        LCDWriteData(usRegAddr);
        LCDWriteData(usValue);
    }

    void WS_9IN7::IT8951LoadImgEnd(void) {
        LCDWriteCmdCode(IT8951::TconLdImgEnd);
    }

    void WS_9IN7::IT8951_BMP_Example(const uint32_t addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        IT8951LdImgInfo stLdImgInfo;
        IT8951AreaImgInfo stAreaImgInfo;

        // Macht ehh nix
        // EPD_Clear(0xff);

        IT8951WaitForDisplayReady();

        //Setting Load image information
        stLdImgInfo.ulStartFBAddr = addr; //(uint32_t) gpFrameBuf;
        stLdImgInfo.usEndianType = IT8951EndianType::Little;
        stLdImgInfo.usPixelFormat = IT8951BppMode::Bpp4;
        stLdImgInfo.usRotate = IT8951RotateMode::Rotate0;
        stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr;
        //Set Load Area
        stAreaImgInfo.usX = x;
        stAreaImgInfo.usY = y;
        stAreaImgInfo.usWidth = w;
        stAreaImgInfo.usHeight = h;

        //Load Image from Host to IT8951 Image Buffer
        IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);  //Display function 2
        //Display Area ?V (x,y,w,h) with mode 2 for fast gray clear mode - depends on current waveform
        //IT8951DisplayArea(0,0, gstI80DevInfo.usPanelW, gstI80DevInfo.usPanelH, 2);
    }

    void WS_9IN7::IT8951WaitForDisplayReady() {
        //Check IT8951 Register LUTAFSR => NonZero Busy, 0 - Free
        while (IT8951ReadReg(RegBaseAddr::LUTAFSR));
    }
/*
    void IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue) {
        //Send Cmd , Register Address and Write Value
        LCDWriteCmdCode(IT8951_TCON_REG_WR);
        LCDWriteData(usRegAddr);
        LCDWriteData(usValue);
    }
*/
    uint16_t WS_9IN7::IT8951ReadReg(uint16_t usRegAddr) {
        uint16_t usData;

        //Send Cmd and Register Address
        LCDWriteCmdCode(IT8951::TconRegRd);
        LCDWriteData(usRegAddr);
        //Read data from Host Data bus
        usData = LCDReadData();
        return usData;
    }

    void WS_9IN7::IT8951SetImgBufBaseAddr(uint32_t ulImgBufAddr) {
        uint16_t usWordH = (uint16_t) ((ulImgBufAddr >> 16) & 0x0000FFFF);
        uint16_t usWordL = (uint16_t) (ulImgBufAddr & 0x0000FFFF);
        //Write LISAR Reg
        IT8951WriteReg(RegBaseAddr::Lisar + 2, usWordH);
        IT8951WriteReg(RegBaseAddr::Lisar, usWordL);
    }

    void WS_9IN7::IT8951LoadImgAreaStart(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo) {
        uint16_t usArg[5];
        //Setting Argument for Load image start
        usArg[0] = (pstLdImgInfo->usEndianType << 8)
                   | (pstLdImgInfo->usPixelFormat << 4)
                   | (pstLdImgInfo->usRotate);
        usArg[1] = pstAreaImgInfo->usX;
        usArg[2] = pstAreaImgInfo->usY;
        usArg[3] = pstAreaImgInfo->usWidth;
        usArg[4] = pstAreaImgInfo->usHeight;
        //Send Cmd and Args
        LCDSendCmdArg(IT8951::TconLdImgArea, usArg, 5);
    }

    void WS_9IN7::IT8951HostAreaPackedPixelWrite(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo) {
        //uint32_t x, y;
        //Source buffer address of Host K
        // uint16_t* pusFrameBuf = (uint16_t*)pstLdImgInfo->ulStartFBAddr;

        // const uint8_t* pusFrameBuf = m_FrameBuffer.GetData()[0].data();

        uint16_t byte;
        uint8_t temp;
        uint16_t extracted;

        // Log::Debug("16b: %i pus: %i", data16b,*pusFrameBuf);
        //ImageData->DrawImage(&m_FrameBuffer, x, y);

        //Set Image buffer(IT8951) Base address
        IT8951SetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);
        //Send Load Image start Cmd
        IT8951LoadImgAreaStart(pstLdImgInfo, pstAreaImgInfo);
        //Host Write Data

        for (int y = 0; y < 400 /*pstAreaImgInfo->usHeigh*/; y++) {
            for (int x = 0; x < 400 /*pstAreaImgInfo->usWidth*/; x += 4) {
                uint16_t pixel = 0;
                for (int i = 3; i >= 0; i--) {
                    uint16_t temp = m_FrameBuffer.GetPixel(x + 3 - i, y);
                    pixel |= temp << (i * 4);
                    if (y < 1 && (x + 3 - i) < 20) {
                        Log::Debug("X: %02i Y: %02i Get: %04x", (x + 3 - i), y, pixel);
                    }
                }
                uint16_t temp1 = (pixel & 0x00ff) << 8;
                uint16_t temp2 = (pixel & 0xff00) >> 8;
                pixel = 0;
                pixel |= temp1;
                pixel |= temp2;

                LCDWriteData(pixel);
            }
        }
        //Send Load Img End Command
        IT8951LoadImgEnd();
    }

    void WS_9IN7::IT8951DisplayArea(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode) {
        //Send I80 Display Command (User defined command of IT8951)
        LCDWriteCmdCode(I80::UsdefCmdDpy_Area);  //0x0034
        //Write arguments
        LCDWriteData(usX);
        LCDWriteData(usY);
        LCDWriteData(usW);
        LCDWriteData(usH);
        LCDWriteData(usDpyMode);
    }




    // ENDE

    WS_9IN7::~WS_9IN7() {
        Log::Debug("[EPDL] Deinitializing WS_9IN7 display driver");
        BeginFrame();
        SwapBuffers();
        EndFrame();
        Log::Debug("[EPDL] WS_9IN7 display driver deinitialized");
    }

    void WS_9IN7::display_buffer(const uint8_t* addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        // gpFrameBuf = addr;
        // Serial.println("Sending image");
        IT8951_BMP_Example(*addr, x, y, w, h);
        // Serial.println("Displaying image");
        IT8951DisplayArea(x, y, w, h, 2);
        // Serial.println("Waiting for display ...");
        WaitUntilReady();
        // Serial.println("done");
    }


    void WS_9IN7::DrawImage(ImageHandle handle, int x, int y) {
        // Init fertig
        // Ab hier Bild laden
        Log::Debug("[EPDL] Drawing image %d at (%d, %d)", handle, x, y);

        for (int i = 0; i < 80000; i++) {
            uint8_t byte = pic[i];
            uint8_t extracted = byte & 0xf0;
            extracted = extracted >> 4;
            m_FrameBuffer.SetPixel((i * 2) % 400, i / 200, extracted);
            if (i < 10) {
                Log::Debug("X: %02i Y: %02i Pix: %02x Set: %02x", (i * 2) % 400, i / 200, byte, extracted);
            }
            extracted = (byte & 0x0f);
            m_FrameBuffer.SetPixel((i * 2 + 1) % 400, i / 200, extracted);

            if (i < 10) {
                Log::Debug("X: %02i Y: %02i Pic: %02x Set: %02x", (i * 2 + 1) % 400, i / 200, byte, extracted);
            }
        }

        // for (int i = 0; i < 80000; i++) {
        //     uint8_t byte = pic[i];
        //     uint8_t extracted = byte & 0x0f;
        //     extracted = extracted;
        //     m_FrameBuffer.SetPixel((i*2)%400, i/200, extracted);
        //     if (i < 10) {
        //             Log::Debug("X: %02i Y: %02i Pix: %02x Set: %02x", (i*2)%400, i/200, byte, extracted);
        //         }
        //     extracted = (byte & 0xf0) >> 4;
        //     m_FrameBuffer.SetPixel((i*2+1)%400, i/200, extracted);

        //     if (i < 10) {
        //             Log::Debug("X: %02i Y: %02i Pic: %02x Set: %02x", (i*2+1)%400, i/200, byte, extracted);
        //         }
        // }

        display_buffer(m_FrameBuffer.GetData()[0].data(), x, y, 400, 400);

        // ImageData* imageData = m_ImageData[handle].get();

    }

    void WS_9IN7::BeginFrame() {
        Log::Debug("[EPDL] Begin frame");
        m_FrameBuffer.ClearColor(Color::White);
        PowerOn();
    }

    void WS_9IN7::EndFrame() {
        Log::Debug("[EPDL] End frame");
        Refresh();
        PowerOff();
        MCU::Sleep(1000);
    }

    void WS_9IN7::SwapBuffers() {
        Log::Debug("[EPDL] Swap buffers");
        uint16_t scale = 8 / m_PixelSize;
        StartDataTransmission();
        for (uint16_t j = 0; j < m_Height; j++) {
            for (uint16_t i = 0; i < m_Width / scale; i++) {
                uint8_t data = 0;
                for (uint8_t k = 0; k < 4; k++) {
                    data |= m_FrameBuffer.GetPixel(i * 4 + k, j) << (6 - k * 2);
                }
                SendData(data);
            }
        }
    }

    void WS_9IN7::SendCommand(uint8_t command) {
        MCU::GPIO::Write(EPDL::Pin::DC, 0);
        m_SPIController->Write(command);
    }

    void WS_9IN7::SendData(uint8_t data) {
        MCU::GPIO::Write(EPDL::Pin::DC, 1);
        m_SPIController->Write(data);
    }

    void WS_9IN7::WaitUntilReady() {
        esp_sleep_enable_timer_wakeup(20 * 1000); // 20ms
        while (MCU::GPIO::Read(EPDL::Pin::BUSY) == 0) {
        esp_light_sleep_start();
        }
    }

    void WS_9IN7::Reset() {
        MCU::GPIO::Write(EPDL::Pin::RST, 0);
        MCU::Sleep(1000);
        MCU::GPIO::Write(EPDL::Pin::RST, 1);
        MCU::Sleep(20);
    }

    void WS_9IN7::PowerOff() {
        WaitUntilReady();
        SendCommand(0x02);
        SendData(0x00);
    }

    void WS_9IN7::PowerOn() {
        SendCommand(0x04);
    }

    void WS_9IN7::Sleep() {
        WaitUntilReady();
        SendCommand(0x07);
        SendData(0xA5);
    }

    void WS_9IN7::StartDataTransmission() {
        WaitUntilReady();
        SendCommand(0x10);
    }

    void WS_9IN7::Refresh() {
        SendCommand(0x12);
        SendData(0x00);
    }
} // namespace EPDL
