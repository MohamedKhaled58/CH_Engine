#pragma warning(disable:4786)
#include "CH_texture.h"
#include "CH_main.h"
#include "CH_datafile.h"
// DirectXTex not available, using simplified approach
#include <wincodec.h>
#include <string>
#include <algorithm>

using namespace DirectX;

// Global texture management (maintaining exact same structure as original)
CH_CORE_DLL_API DWORD g_dwTexCount = 0;
CH_CORE_DLL_API CHTexture* g_lpTex[TEX_MAX];

void Texture_Clear(CHTexture* lpTex)
{
    lpTex->nID = -1;
    lpTex->nDupCount = 0;
    lpTex->lpName = nullptr;
    lpTex->lpTex.Reset();
    lpTex->lpSRV.Reset();
    ZeroMemory(&lpTex->Info, sizeof(CHImageInfo));
    ZeroMemory(&lpTex->d3dDesc, sizeof(D3D11_TEXTURE2D_DESC));
    lpTex->dxgiFormat = DXGI_FORMAT_UNKNOWN;
}

int Texture_Load(CHTexture** lpTex,
                char* lpName,
                DWORD dwMipLevels,
                CHPool pool,
                BOOL bDuplicate,
                DWORD colorkey)
{
    EnterCriticalSection(&g_CriticalSection);

    if (bDuplicate)
    {
        // Search for existing texture with same name
        DWORD add = 0;
        for (int t = 0; t < TEX_MAX; t++)
        {
            if (g_lpTex[t] != nullptr)
            {
                if (_stricmp(g_lpTex[t]->lpName, lpName) == 0)
                {
                    g_lpTex[t]->nDupCount++;
                    *lpTex = g_lpTex[t];
                    return t;
                }
                if (++add == g_dwTexCount)
                    break;
            }
        }
    }

    // Create new texture
    *lpTex = new CHTexture;
    Texture_Clear(*lpTex);
    
    g_objDnFile.BeforeUseDnFile();
    DWORD dwSize = 0;
    void* pBuf = g_objDnFile.GetMPtr(lpName, dwSize);

    BOOL success = FALSE;

    if (pBuf)
    {
        // Load from packed file
        success = CHTextureInternal::CreateTextureFromMemory(pBuf, dwSize, *lpTex, dwMipLevels, colorkey);
        g_objDnFile.AfterUseDnFile();
    }
    else
    {
        // Load from loose file
        g_objDnFile.AfterUseDnFile();
        success = CHTextureInternal::CreateTextureFromFile(lpName, *lpTex, dwMipLevels, colorkey);
    }

    if (!success)
    {
        delete *lpTex;
        *lpTex = nullptr;
        return -1;
    }

    // Set texture name
    size_t nameLen = strlen(lpName) + 1;
    (*lpTex)->lpName = new char[nameLen];
    strcpy_s((*lpTex)->lpName, nameLen, lpName);
    (*lpTex)->nDupCount = 1;

    // Find empty slot in global array
    for (int t = 0; t < TEX_MAX; t++)
    {
        if (g_lpTex[t] == nullptr)
        {
            g_lpTex[t] = *lpTex;
            g_lpTex[t]->nID = t;
            g_dwTexCount++;
            return t;
        }
    }

    // No free slots
    delete[] (*lpTex)->lpName;
    delete *lpTex;
    *lpTex = nullptr;
    return -1;
}

void Texture_Unload(CHTexture** lpTex)
{
    EnterCriticalSection(&g_CriticalSection);

    if (!lpTex || !*lpTex)
    {
        LeaveCriticalSection(&g_CriticalSection);
        return;
    }
        return;

    if (--(*lpTex)->nDupCount <= 0)
    {
        int id = (*lpTex)->nID;

        delete[] (*lpTex)->lpName;
        (*lpTex)->lpTex.Reset();
        (*lpTex)->lpSRV.Reset();
        delete *lpTex;

        if (id >= 0 && id < TEX_MAX)
        {
            g_lpTex[id] = nullptr;
            g_dwTexCount--;
        }
    }

    *lpTex = nullptr;
}

BOOL Texture_Update(CHTexture* lpSrcTex, CHTexture* lpDestTex)
{
    if (!lpSrcTex || !lpDestTex || !lpSrcTex->lpTex || !lpDestTex->lpTex)
        return FALSE;

    // DirectX 11 equivalent of UpdateTexture
    g_D3DContext->CopyResource(lpDestTex->lpTex.Get(), lpSrcTex->lpTex.Get());
    return TRUE;
}

BOOL Texture_Create(CHTexture** lpTex,
                   DWORD dwWidth,
                   DWORD dwHeight,
                   DWORD dwMipLevels,
                   CHFormat format,
                   CHPool pool)
{
    *lpTex = new CHTexture;
    Texture_Clear(*lpTex);

    DXGI_FORMAT dxgiFormat = CHTextureInternal::CHFormatToDXGI(format);
    
    BOOL success = CHTextureInternal::CreateEmptyTexture(dwWidth, dwHeight, dxgiFormat, dwMipLevels, *lpTex);
    
    if (!success)
    {
        delete *lpTex;
        *lpTex = nullptr;
        return FALSE;
    }

    (*lpTex)->Info.Width = dwWidth;
    (*lpTex)->Info.Height = dwHeight;
    (*lpTex)->Info.Format = format;
    (*lpTex)->Info.MipLevels = dwMipLevels;

    return TRUE;
}

// Internal implementation
namespace CHTextureInternal {

DXGI_FORMAT CHFormatToDXGI(CHFormat format)
{
    switch (format)
    {
        case CH_FMT_A8R8G8B8:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case CH_FMT_X8R8G8B8:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        case CH_FMT_R8G8B8:
            return DXGI_FORMAT_R8G8B8A8_UNORM; // No direct equivalent, use RGBA
        case CH_FMT_R5G6B5:
            return DXGI_FORMAT_B5G6R5_UNORM;
        case CH_FMT_X1R5G5B5:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case CH_FMT_A1R5G5B5:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case CH_FMT_A4R4G4B4:
            return DXGI_FORMAT_B4G4R4A4_UNORM;
        case CH_FMT_DXT1:
            return DXGI_FORMAT_BC1_UNORM;
        case CH_FMT_DXT3:
            return DXGI_FORMAT_BC2_UNORM;
        case CH_FMT_DXT5:
            return DXGI_FORMAT_BC3_UNORM;
        default:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

CHFormat DXGIToCHFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return CH_FMT_A8R8G8B8;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return CH_FMT_X8R8G8B8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return CH_FMT_A8R8G8B8;
        case DXGI_FORMAT_B5G6R5_UNORM:
            return CH_FMT_R5G6B5;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return CH_FMT_A1R5G5B5;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return CH_FMT_A4R4G4B4;
        case DXGI_FORMAT_BC1_UNORM:
            return CH_FMT_DXT1;
        case DXGI_FORMAT_BC2_UNORM:
            return CH_FMT_DXT3;
        case DXGI_FORMAT_BC3_UNORM:
            return CH_FMT_DXT5;
        default:
            return CH_FMT_A8R8G8B8;
    }
}

BOOL CreateTextureFromMemory(const void* pData, size_t dataSize, 
                               CHTexture* texture, DWORD mipLevels, DWORD colorKey)
{
    if (!pData || !texture)
        return FALSE;

    // Color key processing and texture creation simplified for compatibility
    // TODO: Implement proper DirectXTex integration when needed
    
    // Create basic texture with placeholder data
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 64;  // Default size
    texDesc.Height = 64;
    texDesc.MipLevels = mipLevels;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    BOOL success = SUCCEEDED(g_D3DDevice->CreateTexture2D(&texDesc, nullptr, texture->lpTex.GetAddressOf()));

    if (!success)
        return FALSE;

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

    BOOL srvSuccess = SUCCEEDED(g_D3DDevice->CreateShaderResourceView(texture->lpTex.Get(), &srvDesc, 
                                             texture->lpSRV.GetAddressOf()));

    if (!srvSuccess)
        return FALSE;

    // Fill texture info
    texture->Info.Width = texDesc.Width;
    texture->Info.Height = texDesc.Height;
    texture->Info.Depth = 1;
    texture->Info.MipLevels = texDesc.MipLevels;
    texture->Info.Format = DXGIToCHFormat(texDesc.Format);
    texture->dxgiFormat = texDesc.Format;

    // Get texture description
    texture->lpTex->GetDesc(&texture->d3dDesc);

    return TRUE;
}

BOOL CreateTextureFromFile(const char* filename, CHTexture* texture, 
                             DWORD mipLevels, DWORD colorKey)
{
    if (!filename || !texture)
        return FALSE;

    // Simplified file loading - create placeholder texture
    // TODO: Implement proper file loading when DirectXTex is available
    
    // Create basic texture with placeholder data
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 64;  // Default size
    texDesc.Height = 64;
    texDesc.MipLevels = mipLevels;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    BOOL success = SUCCEEDED(g_D3DDevice->CreateTexture2D(&texDesc, nullptr, texture->lpTex.GetAddressOf()));

    if (!success)
        return FALSE;

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

    BOOL srvSuccess = SUCCEEDED(g_D3DDevice->CreateShaderResourceView(texture->lpTex.Get(), &srvDesc, 
                                             texture->lpSRV.GetAddressOf()));

    if (!srvSuccess)
        return FALSE;

    // Fill texture info
    texture->Info.Width = texDesc.Width;
    texture->Info.Height = texDesc.Height;
    texture->Info.Depth = 1;
    texture->Info.MipLevels = texDesc.MipLevels;
    texture->Info.Format = DXGIToCHFormat(texDesc.Format);
    texture->dxgiFormat = texDesc.Format;

    // Get texture description
    texture->lpTex->GetDesc(&texture->d3dDesc);

    return TRUE;
}

BOOL CreateEmptyTexture(UINT width, UINT height, DXGI_FORMAT format, 
                          UINT mipLevels, CHTexture* texture)
{
    if (!texture)
        return FALSE;

    // Create texture description
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = mipLevels;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    if (mipLevels > 1)
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    BOOL success = SUCCEEDED(g_D3DDevice->CreateTexture2D(&desc, nullptr, texture->lpTex.GetAddressOf()));
    if (!success)
        return FALSE;

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = mipLevels;

    BOOL srvSuccess = SUCCEEDED(g_D3DDevice->CreateShaderResourceView(texture->lpTex.Get(), &srvDesc, 
                                             texture->lpSRV.GetAddressOf()));

    if (!srvSuccess)
        return FALSE;

    // Store description and info
    texture->d3dDesc = desc;
    texture->dxgiFormat = format;
    texture->Info.Width = width;
    texture->Info.Height = height;
    texture->Info.Depth = 1;
    texture->Info.MipLevels = mipLevels;
    texture->Info.Format = DXGIToCHFormat(format);

    return TRUE;
}

void ProcessColorKey(void* imageData, UINT width, UINT height, 
                    DXGI_FORMAT format, DWORD colorKey)
{
    // This function processes color key transparency
    // Implementation depends on the specific format
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM)
    {
        uint32_t* pixels = reinterpret_cast<uint32_t*>(imageData);
        size_t pixelCount = width * height;
        uint32_t keyColor = colorKey & 0x00FFFFFF;

        for (size_t i = 0; i < pixelCount; i++)
        {
            if ((pixels[i] & 0x00FFFFFF) == keyColor)
            {
                pixels[i] = 0x00000000; // Set to transparent
            }
        }
    }
}

} // namespace CHTextureInternal