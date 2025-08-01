#ifndef _CH_capscreen_h_
#define _CH_capscreen_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"

// Screen capture functions (maintaining exact same signatures as original)
CH_CORE_DLL_API
void CapScreen(char* lpName);

CH_CORE_DLL_API
BOOL JPGEncode(char* lpJpegName,
              DWORD dwWidth,
              DWORD dwHeight,
              unsigned char* lpBuffer,
              int mode = 0,      // 0 = RGB, 1 = BGR
              DWORD dwQuality = 100);

// Internal screen capture implementation
namespace CHCapScreenInternal {
    // Screen capture utilities
    BOOL CaptureBackBuffer(unsigned char** buffer, DWORD* width, DWORD* height);
    BOOL SaveToJPEG(const char* filename, unsigned char* buffer, DWORD width, DWORD height, DWORD quality);
    BOOL SaveToBMP(const char* filename, unsigned char* buffer, DWORD width, DWORD height);
    
    // Format conversion
    void ConvertRGBtoBGR(unsigned char* buffer, DWORD width, DWORD height);
    void FlipImageVertically(unsigned char* buffer, DWORD width, DWORD height, DWORD bytesPerPixel);
}

#endif // _CH_capscreen_h_