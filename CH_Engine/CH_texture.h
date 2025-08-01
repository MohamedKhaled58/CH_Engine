#ifndef _CH_texture_h_
#define _CH_texture_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include <string>

// Remove redundant windows.h include (already in CH_common.h)

// DirectX 11 texture pool types (compatible with DX8 pool types)
enum CHPool {
    CH_POOL_DEFAULT = 0,
    CH_POOL_MANAGED = 1,
    CH_POOL_SYSTEMMEM = 2,
    CH_POOL_SCRATCH = 3
};

// DirectX 11 texture format types (compatible with DX8 formats)
enum CHFormat {
    CH_FMT_UNKNOWN = 0,
    CH_FMT_R8G8B8 = 20,
    CH_FMT_A8R8G8B8 = 21,
    CH_FMT_X8R8G8B8 = 22,
    CH_FMT_R5G6B5 = 23,
    CH_FMT_X1R5G5B5 = 24,
    CH_FMT_A1R5G5B5 = 25,
    CH_FMT_A4R4G4B4 = 26,
    CH_FMT_DXT1 = MAKEFOURCC('D','X','T','1'),
    CH_FMT_DXT3 = MAKEFOURCC('D','X','T','3'),
    CH_FMT_DXT5 = MAKEFOURCC('D','X','T','5')
};

// Image info structure (maintaining compatibility with D3DXIMAGE_INFO)
struct CHImageInfo {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    CHFormat Format;
    UINT ResourceType;
    CHFormat ImageFileFormat;
};

// Texture structure (maintaining exact same layout as C3Texture)
struct CHTexture {
    int nID;                            // Texture ID in global array
    int nDupCount;                      // Reference count
    char* lpName;                       // Filename
    CHComPtr<ID3D11Texture2D> lpTex;    // DirectX 11 texture
    CHComPtr<ID3D11ShaderResourceView> lpSRV; // Shader resource view for binding
    CHImageInfo Info;                   // Texture information
    
    // Internal DirectX 11 specific data
    D3D11_TEXTURE2D_DESC d3dDesc;
    DXGI_FORMAT dxgiFormat;
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Texture_Clear(CHTexture* lpTex);

/*
    Load texture
    ------------
    lpTex       - Texture pointer
    lpName      - Filename
    dwMipLevels - Mipmap levels (default 3)
    pool        - Memory pool (compatibility parameter)
    bDuplicate  - Enable texture sharing
    colorkey    - Color key for transparency
*/
CH_CORE_DLL_API
int Texture_Load(CHTexture** lpTex,
                const char* lpName,
                DWORD dwMipLevels = 3,
                CHPool pool = CH_POOL_MANAGED,
                BOOL bDuplicate = TRUE,
                DWORD colorkey = 0);

CH_CORE_DLL_API
void Texture_Unload(CHTexture** lpTex);

CH_CORE_DLL_API
BOOL Texture_Update(CHTexture* lpSrcTex, CHTexture* lpDestTex);

CH_CORE_DLL_API
BOOL Texture_Create(CHTexture** lpTex,
                   DWORD dwWidth,
                   DWORD dwHeight,
                   DWORD dwMipLevels,
                   CHFormat format,
                   CHPool pool);

// Global texture management (maintaining exact same interface)
#define TEX_MAX 10240
extern CH_CORE_DLL_API DWORD g_dwTexCount;
extern CH_CORE_DLL_API CHTexture* g_lpTex[TEX_MAX];

// Modern C++ helper functions (internal use)
namespace CHTextureInternal {
    // Format conversion utilities
    DXGI_FORMAT CHFormatToDXGI(CHFormat format);
    CHFormat DXGIToCHFormat(DXGI_FORMAT format);

    // Texture loading from memory
    BOOL CreateTextureFromMemory(const void* pData, size_t dataSize,
        CHTexture* texture, DWORD mipLevels,
        DWORD colorKey);
    BOOL CreateTextureFromPixels(const DWORD* pixels, UINT width, UINT height, CHTexture* texture);

    BOOL CreateDefaultTexture(UINT width, UINT height, CHTexture* texture);

    // Texture loading from file
    BOOL CreateTextureFromFile(const char* filename, CHTexture* texture,
        DWORD mipLevels, DWORD colorKey);

    // DirectX 11 texture creation
    BOOL CreateEmptyTexture(UINT width, UINT height, DXGI_FORMAT format,
        UINT mipLevels, CHTexture* texture);

    // Color key processing for transparency
    void ProcessColorKey(void* imageData, UINT width, UINT height,
        DXGI_FORMAT format, DWORD colorKey);
}

// Compatibility types for external code
typedef CHTexture C3Texture; // For backwards compatibility
//typedef CHFormat D3DFORMAT;  // For backwards compatibility
//typedef CHPool D3DPOOL;      // For backwards compatibility

#endif // _CH_texture_h_