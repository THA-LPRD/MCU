#include "WS_9IN7.h"
#include "Log.h"
#include <MCU.h>
#include <GPIO.h>
#include <SPI.h>

namespace EPDL
{
    WS_9IN7::WS_9IN7() : m_FrameBuffer(m_Width, m_Height, m_PixelSize) {
        Log::Debug("[EPDL] Initializing WS_9IN7 display driver");
        // MISO Pin is labeled DC on EPD
        m_SPIController = MCU::SPI::Create(static_cast<MCU::SPIDevice>(EPDL::SPI::SPIDevice),
            EPDL::Pin::MOSI,
            EPDL::Pin::DC,
            EPDL::Pin::SCK,
            EPDL::Pin::CS,
            false);
        // Lesezeichen
        // Software reset
        Reset();

        // GetDisplayInfo();










        // Initialize Display Register
        SendCommand(0xAA);
        SendData(0x49);
        SendData(0x55);
        SendData(0x20);
        SendData(0x08);
        SendData(0x09);
        SendData(0x18);

        SendCommand(0x01);
        SendData(0x3F);

        SendCommand(0x00);
        SendData(0x4F);
        SendData(0x69);

        SendCommand(0x05);
        SendData(0x40);
        SendData(0x1F);
        SendData(0x1F);
        SendData(0x2C);

        SendCommand(0x08);
        SendData(0x6F);
        SendData(0x1F);
        SendData(0x1F);
        SendData(0x22);

        SendCommand(0x06);
        SendData(0x6F);
        SendData(0x1F);
        SendData(0x14);
        SendData(0x14);

        SendCommand(0x03);
        SendData(0x00);
        SendData(0x54);
        SendData(0x00);
        SendData(0x44);

        SendCommand(0x60);
        SendData(0x02);
        SendData(0x00);

        SendCommand(0x30);
        SendData(0x08);

        SendCommand(0x50);
        SendData(0x3F);

        SendCommand(0x61);
        SendData(0x03);
        SendData(0x20);
        SendData(0x01);
        SendData(0xE0);

        SendCommand(0xE3);
        SendData(0x2F);

        SendCommand(0x84);
        SendData(0x01);

        BeginFrame();
        SwapBuffers();
        EndFrame();

        Log::Debug("[EPDL] WS_9IN7 display driver initialized");
        // }



            // Init
        GetIT8951SystemInfo(&gstI80DevInfo);

        gulImgBufAddr = gstI80DevInfo.usImgBufAddrL | ((uint32_t)gstI80DevInfo.usImgBufAddrH << 16);

        //Set to Enable I80 Packed mode
        IT8951WriteReg(I80CPCR, 0x0001);


        // display_buffer

        gpFrameBuf = addr;
        // Serial.println("Sending image");
        IT8951_BMP_Example(x, y, w, h);
        // Serial.println("Displaying image");
        IT8951DisplayArea(x, y, w, h, 2);
        // Serial.println("Waiting for display ...");
        LCDWaitForReady();
        // Serial.println("done");
    }


    void GetIT8951SystemInfo(void* pBuf) {
        uint16_t* pusWord = (uint16_t*)pBuf;
        IT8951DevInfo* pstDevInfo;

        //Send I80 CMD
        LCDWriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO);

        //Burst Read Request for SPI interface only
        LCDReadNData(pusWord, sizeof(IT8951DevInfo) / 2);  //Polling HRDY for each words(2-bytes) if possible

        //Show Device information of IT8951
        pstDevInfo = (IT8951DevInfo*)pBuf;
        printf("Panel(W,H) = (%d,%d)\r\n",
            pstDevInfo->usPanelW, pstDevInfo->usPanelH);
        printf("Image Buffer Address = %X\r\n",
            pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
        //Show Firmware and LUT Version
        printf("FW Version = %s\r\n", (uint8_t*)pstDevInfo->usFWVersion);
        printf("LUT Version = %s\r\n", (uint8_t*)pstDevInfo->usLUTVersion);
    }

    void LCDWriteCmdCode(uint16_t usCmdCode) {
        //Set Preamble for Write Command
        uint16_t wPreamble = 0x6000;

        LCDWaitForReady();

        bcm2835_gpio_write(CS, LOW);

        bcm2835_spi_transfer(wPreamble >> 8);
        bcm2835_spi_transfer(wPreamble);

        LCDWaitForReady();

        bcm2835_spi_transfer(usCmdCode >> 8);
        bcm2835_spi_transfer(usCmdCode);

        bcm2835_gpio_write(CS, HIGH);
    }

    void LCDSendCmdArg(uint16_t usCmdCode, uint16_t* pArg, uint16_t usNumArg) {
        uint16_t i;
        //Send Cmd code
        LCDWriteCmdCode(usCmdCode);
        //Send Data
        for (i = 0; i < usNumArg; i++) {
            LCDWriteData(pArg[i]);
        }
    }

    void LCDWriteData(uint16_t usData) {
        //Set Preamble for Write Data
        uint16_t wPreamble = 0x0000;

        LCDWaitForReady();

        bcm2835_gpio_write(CS, LOW);

        bcm2835_spi_transfer(wPreamble >> 8);
        bcm2835_spi_transfer(wPreamble);

        LCDWaitForReady();

        bcm2835_spi_transfer(usData >> 8);
        bcm2835_spi_transfer(usData);

        bcm2835_gpio_write(CS, HIGH);
    }

    void LCDReadNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt) {
        uint32_t i;

        uint16_t wPreamble = 0x1000;

        LCDWaitForReady();

        bcm2835_gpio_write(CS, LOW);

        bcm2835_spi_transfer(wPreamble >> 8);
        bcm2835_spi_transfer(wPreamble);

        LCDWaitForReady();

        pwBuf[0] = bcm2835_spi_transfer(0x00);  //dummy
        pwBuf[0] = bcm2835_spi_transfer(0x00);  //dummy

        LCDWaitForReady();

        for (i = 0; i < ulSizeWordCnt; i++) {
            pwBuf[i] = bcm2835_spi_transfer(0x00) << 8;
            pwBuf[i] |= bcm2835_spi_transfer(0x00);
        }

        bcm2835_gpio_write(CS, HIGH);
    }

    uint16_t LCDReadData() {
        uint16_t wRData;

        uint16_t wPreamble = 0x1000;

        LCDWaitForReady();

        bcm2835_gpio_write(CS, LOW);

        bcm2835_spi_transfer(wPreamble >> 8);
        bcm2835_spi_transfer(wPreamble);

        LCDWaitForReady();

        wRData = bcm2835_spi_transfer(0x00);  //dummy
        wRData = bcm2835_spi_transfer(0x00);  //dummy

        LCDWaitForReady();

        wRData = bcm2835_spi_transfer(0x00) << 8;
        wRData |= bcm2835_spi_transfer(0x00);

        bcm2835_gpio_write(CS, HIGH);

        return wRData;
    }

    void LCDWaitForReady() {
        uint8_t ulData = bcm2835_gpio_lev(HRDY);
        while (ulData == 0) {
            ulData = bcm2835_gpio_lev(HRDY);
        }
    }

    void EPD_Clear(uint8_t Color) {
        //memset(gpFrameBuf, Color, gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH);
    }

    void IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue) {
        //Send Cmd , Register Address and Write Value
        LCDWriteCmdCode(IT8951_TCON_REG_WR);
        LCDWriteData(usRegAddr);
        LCDWriteData(usValue);
    }

    void IT8951LoadImgEnd(void) {
        LCDWriteCmdCode(IT8951_TCON_LD_IMG_END);
    }

    void IT8951_BMP_Example(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        IT8951LdImgInfo stLdImgInfo;
        IT8951AreaImgInfo stAreaImgInfo;

        EPD_Clear(0xff);

        IT8951WaitForDisplayReady();

        //Setting Load image information
        stLdImgInfo.ulStartFBAddr = (uint32_t)gpFrameBuf;
        stLdImgInfo.usEndianType = IT8951_LDIMG_L_ENDIAN;
        stLdImgInfo.usPixelFormat = IT8951_4BPP;
        stLdImgInfo.usRotate = IT8951_ROTATE_0;
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

    void IT8951WaitForDisplayReady() {
        //Check IT8951 Register LUTAFSR => NonZero Busy, 0 - Free
        while (IT8951ReadReg(LUTAFSR))
            ;
    }

    void IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue) {
        //Send Cmd , Register Address and Write Value
        LCDWriteCmdCode(IT8951_TCON_REG_WR);
        LCDWriteData(usRegAddr);
        LCDWriteData(usValue);
    }

    uint16_t IT8951ReadReg(uint16_t usRegAddr) {
        uint16_t usData;

        //Send Cmd and Register Address
        LCDWriteCmdCode(IT8951_TCON_REG_RD);
        LCDWriteData(usRegAddr);
        //Read data from Host Data bus
        usData = LCDReadData();
        return usData;
    }

    void IT8951SetImgBufBaseAddr(uint32_t ulImgBufAddr) {
        uint16_t usWordH = (uint16_t)((ulImgBufAddr >> 16) & 0x0000FFFF);
        uint16_t usWordL = (uint16_t)(ulImgBufAddr & 0x0000FFFF);
        //Write LISAR Reg
        IT8951WriteReg(LISAR + 2, usWordH);
        IT8951WriteReg(LISAR, usWordL);
    }

    void IT8951LoadImgAreaStart(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo) {
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
        LCDSendCmdArg(IT8951_TCON_LD_IMG_AREA, usArg, 5);
    }

    void IT8951HostAreaPackedPixelWrite(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo) {
        uint32_t i, j;
        //Source buffer address of Host
        uint16_t* pusFrameBuf = (uint16_t*)pstLdImgInfo->ulStartFBAddr;

        //Set Image buffer(IT8951) Base address
        IT8951SetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);
        //Send Load Image start Cmd
        IT8951LoadImgAreaStart(pstLdImgInfo, pstAreaImgInfo);
        //Host Write Data
        for (j = 0; j < pstAreaImgInfo->usHeight; j++) {
            for (i = 0; i < pstAreaImgInfo->usWidth / 2; i++) {
                //Write a Word(2-Bytes) for each time
                LCDWriteData(*pusFrameBuf);
                pusFrameBuf++;
            }
        }
        //Send Load Img End Command
        IT8951LoadImgEnd();
    }

    void IT8951DisplayArea(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode) {
        //Send I80 Display Command (User defined command of IT8951)
        LCDWriteCmdCode(USDEF_I80_CMD_DPY_AREA);  //0x0034
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

    void WS_9IN7::DrawImage(ImageHandle handle, int x, int y) {
        Log::Debug("[EPDL] Drawing image %d at (%d, %d)", handle, x, y);
        ImageData* imageData = m_ImageData[handle].get();
        for (uint16_t j = 0; j < imageData->GetHeight(); j++) {
            for (uint16_t i = 0; i < imageData->GetWidth(); i++) {
                if (i + x >= m_Width || j + y >= m_Height) {
                    continue;
                }
                m_FrameBuffer.SetPixel(i + x, j + y, imageData->GetPixel(i, j));
            }
        }
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
        //LOW: idle, HIGH: busy
        Log::Debug("[EPDL] Waiting for display");


        while (MCU::GPIO::Read(EPDL::Pin::BUSY) == 0) {
            MCU::Sleep(5);
        }
        Log::Debug("[EPDL] Display ready");
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
