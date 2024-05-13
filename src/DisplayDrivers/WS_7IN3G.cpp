#include <cstdlib>
#include <utility/EPD_7in3g.h>
#include "DisplayDrivers/WS_7IN3G.h"
#include "Log.h"
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"

namespace EPDL
{
    WS_7IN3G::WS_7IN3G() {
        Log::Debug("Initializing WS_7IN3G display driver");
        // EPD_7IN3G_Init();
        // EPD_7IN3G_Clear(EPD_7IN3G_WHITE);
    }

    void WS_7IN3G::DrawImage(ImageHandle handle, int x, int y) {
        ImageData* imageData = m_ImageData[handle].get();
        uint16_t imgWidth = imageData->Width;
        uint16_t imgHeight = imageData->Height;

        int startX = std::max(0, x);
        int startY = std::max(0, y);
        int endX = std::min(x + imgWidth, EPD_7IN3G_WIDTH);
        int endY = std::min(y + imgHeight, EPD_7IN3G_HEIGHT);

        uint16_t destWidthByte = (EPD_7IN3G_WIDTH % 4 == 0) ?
            (EPD_7IN3G_WIDTH / 4) :
            (EPD_7IN3G_WIDTH / 4 + 1);
        uint16_t srcWidthByte = (imgWidth % 4 == 0) ?
            (imgWidth / 4) :
            (imgWidth / 4 + 1);

        for (int destY = startY; destY < endY; destY++) {
            for (int destX = startX; destX < endX; destX++) {
                uint32_t srcAddr = (destX - x) + (destY - y) * srcWidthByte;
                uint32_t destAddr = destX + destY * destWidthByte;

                if (srcAddr < imageData->Data.size()) {
                    m_FrameBuffer.Data[destAddr] = imageData->Data.at(srcAddr);
                }
            }
        }
    }

    void WS_7IN3G::BeginFrame() {
        size_t bufferSize = EPD_7IN3G_WIDTH * EPD_7IN3G_HEIGHT;
        m_FrameBuffer.Data.resize(bufferSize, EPD_7IN3G_WHITE);
        Log::Debug("Frame buffer size: %d", m_FrameBuffer.Data.size());
    }

    void WS_7IN3G::EndFrame() {

    }

    static void EPD_7IN3G_SendCommand(UBYTE Reg) {
        DEV_Digital_Write(EPD_DC_PIN, 0);
        DEV_Digital_Write(EPD_CS_PIN, 0);
        DEV_SPI_WriteByte(Reg);
        DEV_Digital_Write(EPD_CS_PIN, 1);
    }

    static void EPD_7IN3G_ReadBusyH(void) {
        WS_INTERNAL_DEBUG("e-Paper busy H\r\n");
        while (!DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: idle, HIGH: busy
            DEV_Delay_ms(5);
        }
        WS_INTERNAL_DEBUG("e-Paper busy H release\r\n");
    }

    static void EPD_7IN3G_SendData(UBYTE Data) {
        DEV_Digital_Write(EPD_DC_PIN, 1);
        DEV_Digital_Write(EPD_CS_PIN, 0);
        DEV_SPI_WriteByte(Data);
        DEV_Digital_Write(EPD_CS_PIN, 1);
    }

    static void EPD_7IN3G_TurnOnDisplay(void) {
        EPD_7IN3G_SendCommand(0x12); // DISPLAY_REFRESH
        EPD_7IN3G_SendData(0x00);
        EPD_7IN3G_ReadBusyH();

        EPD_7IN3G_SendCommand(0x02); // POWER_OFF
        EPD_7IN3G_SendData(0X00);
        EPD_7IN3G_ReadBusyH();
    }

    void WS_7IN3G::SwapBuffers() {
        EPD_7IN3G_Display(m_FrameBuffer.Data.data());

        uint16_t Width = (EPD_7IN3G_WIDTH % 4 == 0) ? (EPD_7IN3G_WIDTH / 4) : (EPD_7IN3G_WIDTH / 4 + 1);
        uint16_t Height = EPD_7IN3G_HEIGHT;

        EPD_7IN3G_SendCommand(0x04);
        EPD_7IN3G_ReadBusyH();

        EPD_7IN3G_SendCommand(0x10);
        for (UWORD j = 0; j < Height; j++) {
            for (UWORD i = 0; i < Width; i++) {
                EPD_7IN3G_SendData(Image[i + j * Width]);
            }
        }
        EPD_7IN3G_TurnOnDisplay();
    }

} // namespace EPDL
