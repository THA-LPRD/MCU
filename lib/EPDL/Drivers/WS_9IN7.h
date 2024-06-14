#ifndef WS_9IN7_H_
#define WS_9IN7_H_

#include "Driver.h"
#include "FrameBuffer.h"

namespace EPDL
{
    class WS_9IN7 : public EPDL::Driver {
    public:
        enum Color : uint8_t {
            Black = 0x0,
            White = 0x1,
            Yellow = 0x2,
            Red = 0x3
        };
        WS_9IN7();
        ~WS_9IN7();
        void DrawImage(ImageHandle handle, int x, int y) override;
        void BeginFrame() override;
        void EndFrame() override;
        void SwapBuffers() override;
        uint16_t GetWidth() const override { return m_Width; }
        uint16_t GetHeight() const override { return m_Height; }
    private:
        enum I80Cmd {
            DpyArea = 0x0034,
            DevInfo = 0x0302,
            DpyBufArea = 0x0037,
        };

        enum I80 {
            SysRegBase = 0x0000,
            Cpcr = (SysRegBase + 0x04),
            //I80 User defined command code
            UsdefCmdDpy_Area = 0x0034,
            UsdefCmdGet_Dev_Info = 0x0302,
            UsdefCmdDPY_Buf_Area = 0x0037,
        };

        enum IT8951 {
            TconRegWr = 0x0011,
            TconRegRd = 0x0010,
            TconLdImgArea = 0x0021,
            TconLdImgEnd = 0x0022,
        };

        enum IT8951EndianType {
            Little = 0,
            Big = 1,
        };

        enum IT8951RotateMode {
            //Rotate mode
            Rotate0 = 0,
            Rotate90 = 1,
            Rotate180 = 2,
            Rotate270 = 3,
        };

        enum IT8951BppMode {
            //Pixel mode , BPP - Bit per Pixel
            Bpp2 = 0,
            Bpp3 = 1,
            Bpp4 = 2,
            Bpp8 = 3,
        };

        enum RegBaseAddr {
            //Register Base Address
            DisplayRegBase = 0x1000,  //Register RW access for I80 only
            //Base Address of Basic LUT Registers
            LUT0EWHR = (DisplayRegBase + 0x00),   //LUT0 Engine Width Height Reg
            LUT0XYR = (DisplayRegBase + 0x40),    //LUT0 XY Reg
            LUT0BADDR = (DisplayRegBase + 0x80),  //LUT0 Base Address Reg
            LUT0MFN = (DisplayRegBase + 0xC0),    //LUT0 Mode and Frame number Reg
            LUT01AF = (DisplayRegBase + 0x114),   //LUT0 and LUT1 Active Flag Reg
            //Update Parameter Setting Register
            UP0SR = (DisplayRegBase + 0x134),  //Update Parameter0 Sett
            UP1SR = (DisplayRegBase + 0x138),      //Update Parameter1 Setting Reg
            LUT0ABFRV = (DisplayRegBase + 0x13C),  //LUT0 Alpha blend and Fill rectangle Value
            UPBBADDR = (DisplayRegBase + 0x17C),   //Update Buffer Base Address
            LUT0IMXY = (DisplayRegBase + 0x180),   //LUT0 Image buffer X/Y offset Reg
            LUTAFSR = (DisplayRegBase + 0x224),    //LUT Status Reg (status of All LUT Engines)

            //-------Memory Converter Registers----------------
            Mcsr_Base_Addr = 0x0200,
            Mcsr = (Mcsr_Base_Addr + 0x0000),
            Lisar = (Mcsr_Base_Addr + 0x0008),
        };

        struct IT8951DevInfo {
            uint16_t usPanelW;
            uint16_t usPanelH;
            uint16_t usImgBufAddrL;
            uint16_t usImgBufAddrH;
            uint16_t usFWVersion[8];   //16 Bytes String
            uint16_t usLUTVersion[8];  //16 Bytes String
        };

        struct IT8951AreaImgInfo {
            uint16_t usX;
            uint16_t usY;
            uint16_t usWidth;
            uint16_t usHeight;
        };

        struct IT8951LdImgInfo {
            uint16_t usEndianType;      //little or Big Endian
            uint16_t usPixelFormat;     //bpp
            uint16_t usRotate;          //Rotate mode
            uint32_t ulStartFBAddr;     //Start address of source Frame buffer
            uint32_t ulImgBufBaseAddr;  //Base address of target image buffer

        };

    private:
        void SendCommand(uint8_t command);
        void SendData(uint8_t data);
        uint8_t ReadData(uint8_t data);
        void WaitUntilReady();
        void Reset();
        void PowerOff();
        void PowerOn();
        void Sleep();
        void StartDataTransmission();
        void Refresh();
        void GetIT8951SystemInfo(void* pBuf);
        void LCDWriteCmdCode(uint16_t usCmdCode);
        void LCDReadNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt);
        void IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue);
        void LCDWriteData(uint16_t usData);
        void LCDWaitForReady();
        void display_buffer(const uint8_t* addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        void IT8951_BMP_Example(const uint32_t addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        void IT8951WaitForDisplayReady();
        void IT8951HostAreaPackedPixelWrite(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo);
        void IT8951SetImgBufBaseAddr(uint32_t ulImgBufAddr);
        void IT8951LoadImgAreaStart(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo);
        void LCDSendCmdArg(uint16_t usCmdCode, uint16_t* pArg, uint16_t usNumArg);
        uint16_t LCDReadData();
        uint16_t IT8951ReadReg(uint16_t usRegAddr);
        void IT8951LoadImgEnd(void);
        void IT8951DisplayArea(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode);
        uint16_t m_Width = 1200;
        uint16_t m_Height = 825;
        uint8_t m_PixelSize = 4;
        FrameBuffer m_FrameBuffer;
        // IT8951DevInfo gstI80DevInfo;

    };
} // namespace EPDL

#endif //WS_9IN7_H_
