#ifndef WS_7IN3G_H_
#define WS_7IN3G_H_

#include "Driver.h"

namespace EPDL
{
    class WS_7IN3G : public EPDL::Driver {
    public:
        WS_7IN3G();
        ~WS_7IN3G() = default;
        void DrawImage(ImageHandle handle, int x = 0, int y = 0) override;
        void BeginFrame() override;
        void EndFrame() override;
        void SwapBuffers() override;
    private:
        ImageHandle m_EmptyImage;
    };
} // namespace EPDL

#endif /*WS_7IN3G_H_*/
