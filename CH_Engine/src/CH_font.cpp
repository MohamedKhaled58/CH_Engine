#include "CH_font.h"
#include "CH_main.h"

// Font size adjustment function (from original) - moved to namespace

void Char_Clear(CHChar* lpChar)
{
    if (!lpChar)
        return;
        
    lpChar->Char[0] = '\0';
    lpChar->Char[1] = '\0';
    lpChar->lpTex = nullptr;
    lpChar->dwTime = 0;
}

void Font_Clear(CHFont* lpFont)
{
    if (!lpFont)
        return;
        
    // Clear font bitmap buffer
    if (lpFont->lpBuffer)
    {
        delete[] lpFont->lpBuffer;
        lpFont->lpBuffer = nullptr;
    }
    
    // Release Windows font objects
    if (lpFont->bitmap)
    {
        DeleteObject(lpFont->bitmap);
        lpFont->bitmap = nullptr;
    }
    
    if (lpFont->font)
    {
        DeleteObject(lpFont->font);
        lpFont->font = nullptr;
    }
    
    if (lpFont->hDC)
    {
        DeleteDC(lpFont->hDC);
        lpFont->hDC = nullptr;
    }
    
    // Clear character cache
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            if (lpFont->lpChar[i][j])
            {
                if (lpFont->lpChar[i][j]->lpTex)
                {
                    Texture_Unload(&lpFont->lpChar[i][j]->lpTex);
                }
                delete lpFont->lpChar[i][j];
                lpFont->lpChar[i][j] = nullptr;
            }
        }
    }
    
    ZeroMemory(lpFont->szName, sizeof(lpFont->szName));
    lpFont->nSize = 0;
    lpFont->nRealSize = 0;
    lpFont->dwChars = 0;
}

BOOL Font_Create(CHFont** lpFont, const char* lpFontName, int nSize)
{
    if (!lpFont || !lpFontName)
        return FALSE;
    
    *lpFont = new CHFont;
    
    // Initialize the font structure to zero before clearing
    ZeroMemory(*lpFont, sizeof(CHFont));
    
    // Now it's safe to call Font_Clear
    Font_Clear(*lpFont);
    
    // Store font information
    strncpy_s((*lpFont)->szName, sizeof((*lpFont)->szName), lpFontName, _TRUNCATE);
    (*lpFont)->nSize = nSize;
    (*lpFont)->nRealSize = CHFontInternal::GetFontRealSize(nSize);
    
    // Create Windows font
    (*lpFont)->font = CreateFontA(
        (*lpFont)->nRealSize,   // Height
        0,                      // Width (0 = default)
        0,                      // Escapement
        0,                      // Orientation
        FW_NORMAL,              // Weight
        FALSE,                  // Italic
        FALSE,                  // Underline
        FALSE,                  // Strikeout
        ANSI_CHARSET,           // Charset
        OUT_DEFAULT_PRECIS,     // Output precision
        CLIP_DEFAULT_PRECIS,    // Clipping precision
        ANTIALIASED_QUALITY,    // Quality
        DEFAULT_PITCH | FF_DONTCARE, // Pitch and family
        lpFontName              // Font name
    );
    
    if (!(*lpFont)->font)
    {
        Font_Release(lpFont);
        return FALSE;
    }
    
    // Create device context for font rendering
    HDC screenDC = GetDC(nullptr);
    (*lpFont)->hDC = CreateCompatibleDC(screenDC);
    ReleaseDC(nullptr, screenDC);
    
    if (!(*lpFont)->hDC)
    {
        Font_Release(lpFont);
        return FALSE;
    }
    
    // Select font into DC
    SelectObject((*lpFont)->hDC, (*lpFont)->font);
    SetTextColor((*lpFont)->hDC, RGB(255, 255, 255));
    SetBkColor((*lpFont)->hDC, RGB(0, 0, 0));
    SetBkMode((*lpFont)->hDC, OPAQUE);
    
    // Create bitmap for character rendering
    int bitmapSize = (*lpFont)->nRealSize * (*lpFont)->nRealSize * 4; // RGBA
    (*lpFont)->lpBuffer = new DWORD[bitmapSize / 4];
    
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (*lpFont)->nRealSize;
    bmi.bmiHeader.biHeight = -(*lpFont)->nRealSize; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* bitmapBits;
    (*lpFont)->bitmap = CreateDIBSection((*lpFont)->hDC, &bmi, DIB_RGB_COLORS, &bitmapBits, nullptr, 0);
    
    if (!(*lpFont)->bitmap)
    {
        Font_Release(lpFont);
        return FALSE;
    }
    
    SelectObject((*lpFont)->hDC, (*lpFont)->bitmap);
    
    return TRUE;
}

void Font_Release(CHFont** lpFont)
{
    if (!lpFont || !*lpFont)
        return;
        
    Font_Clear(*lpFont);
    delete *lpFont;
    *lpFont = nullptr;
}

void Font_Prepare()
{
    CHFontInternal::SetupFontRenderStates();
}

BOOL Font_Draw(CHFont* lpFont, float fX, float fY, DWORD color, char* lpChar)
{
    if (!lpFont || !lpChar)
        return FALSE;
    
    int len = static_cast<int>(strlen(lpChar));
    float currentX = fX;
    
    for (int i = 0; i < len; i++)
    {
        unsigned char ch = static_cast<unsigned char>(lpChar[i]);
        
        // Get or create cached character
        CHChar* cachedChar = CHFontInternal::GetCachedCharacter(lpFont, ch);
        if (!cachedChar)
        {
            CHFontInternal::CacheCharacter(lpFont, ch);
            cachedChar = CHFontInternal::GetCachedCharacter(lpFont, ch);
        }
        
        if (cachedChar && cachedChar->lpTex)
        {
            // Create quad vertices for character
            CHFontVertex vertices[4];
            float charWidth = static_cast<float>(lpFont->nRealSize);
            float charHeight = static_cast<float>(lpFont->nRealSize);
            
            // Top-left
            vertices[0].x = currentX;
            vertices[0].y = fY;
            vertices[0].z = 0.0f;
            vertices[0].rhw = 1.0f;
            vertices[0].color = color;
            vertices[0].u = 0.0f;
            vertices[0].v = 0.0f;
            
            // Top-right
            vertices[1].x = currentX + charWidth;
            vertices[1].y = fY;
            vertices[1].z = 0.0f;
            vertices[1].rhw = 1.0f;
            vertices[1].color = color;
            vertices[1].u = 1.0f;
            vertices[1].v = 0.0f;
            
            // Bottom-left
            vertices[2].x = currentX;
            vertices[2].y = fY + charHeight;
            vertices[2].z = 0.0f;
            vertices[2].rhw = 1.0f;
            vertices[2].color = color;
            vertices[2].u = 0.0f;
            vertices[2].v = 1.0f;
            
            // Bottom-right
            vertices[3].x = currentX + charWidth;
            vertices[3].y = fY + charHeight;
            vertices[3].z = 0.0f;
            vertices[3].rhw = 1.0f;
            vertices[3].color = color;
            vertices[3].u = 1.0f;
            vertices[3].v = 1.0f;
            
            // Render character quad
            CHFontInternal::RenderTextQuad(vertices, cachedChar->lpTex);
            
            currentX += charWidth * 0.6f; // Character spacing
        }
    }
    
    return TRUE;
}

// Internal implementation
namespace CHFontInternal {

BOOL CreateCharacterTexture(CHFont* font, CHChar* character, char ch)
{
    if (!font || !character)
        return FALSE;
    
    // Clear bitmap
    RECT rect = { 0, 0, font->nRealSize, font->nRealSize };
    FillRect(font->hDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    
    // Draw character using Windows GDI
    char charStr[2] = { ch, '\0' };
    TextOutA(font->hDC, 0, 0, charStr, 1);
    
    // Get bitmap data
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = font->nRealSize;
    bmi.bmiHeader.biHeight = -font->nRealSize;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    GetDIBits(font->hDC, font->bitmap, 0, font->nRealSize, font->lpBuffer, &bmi, DIB_RGB_COLORS);
    
    // Create texture from bitmap data
    CHTexture** charTexture = &character->lpTex;
    if (Texture_Create(charTexture, font->nRealSize, font->nRealSize, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED))
    {
        // Convert grayscale to alpha channel
        DWORD* pixels = font->lpBuffer;
        for (int i = 0; i < font->nRealSize * font->nRealSize; i++)
        {
            DWORD pixel = pixels[i];
            BYTE intensity = (BYTE)(pixel & 0xFF); // Get blue component as intensity
            pixels[i] = (intensity << 24) | 0x00FFFFFF; // Set alpha, white color
        }
        
        // Update texture with processed data using DirectX 11
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = g_D3DContext->Map((*charTexture)->lpTex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            // Copy pixel data
            DWORD* destPixels = static_cast<DWORD*>(mappedResource.pData);
            for (UINT y = 0; y < font->nRealSize; y++) {
                memcpy(destPixels + y * (mappedResource.RowPitch / 4), 
                       pixels + y * font->nRealSize, 
                       font->nRealSize * sizeof(DWORD));
            }
            g_D3DContext->Unmap((*charTexture)->lpTex.Get(), 0);
        }
        
        return TRUE;
    }
    
    return FALSE;
}

void CacheCharacter(CHFont* font, char ch)
{
    if (!font || font->lpChar[ch][0])
        return; // Already cached
    
    CHChar* newChar = new CHChar;
    Char_Clear(newChar);
    newChar->Char[0] = ch;
    newChar->dwTime = static_cast<DWORD>(GetTickCount64());
    
    if (CreateCharacterTexture(font, newChar, ch))
    {
        font->lpChar[ch][0] = newChar;
        font->dwChars++;
    }
    else
    {
        delete newChar;
    }
}

CHChar* GetCachedCharacter(CHFont* font, char ch)
{
    if (!font)
        return nullptr;
        
    CHChar* cachedChar = font->lpChar[ch][0];
    if (cachedChar)
    {
        cachedChar->dwTime = static_cast<DWORD>(GetTickCount64()); // Update access time
    }
    
    return cachedChar;
}

void SetupFontRenderStates()
{
    // Set render states for font rendering
    SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
    SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCALPHA);
    SetRenderState(CH_RS_DESTBLEND, CH_BLEND_INVSRCALPHA);
    SetRenderState(CH_RS_ZENABLE, FALSE);
    SetRenderState(CH_RS_CULLMODE, CH_CULL_NONE);
}

BOOL RenderTextQuad(CHFontVertex* vertices, CHTexture* texture)
{
    if (!vertices || !texture)
        return FALSE;
    
    // Set texture
    SetTexture(0, texture->lpSRV.Get());
    
    // Create temporary vertex buffer for immediate rendering
    // In a full implementation, we'd use a persistent dynamic vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(CHFontVertex) * 4;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    CHComPtr<ID3D11Buffer> vertexBuffer;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    
    if (FAILED(g_D3DDevice->CreateBuffer(&bufferDesc, &initData, vertexBuffer.GetAddressOf())))
        return FALSE;
    
    // Set vertex buffer
    UINT stride = sizeof(CHFontVertex);
    UINT offset = 0;
    g_D3DContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    // Draw quad
    g_D3DContext->Draw(4, 0);
    
    return TRUE;
}

int GetFontRealSize(int requestedSize)
{
    if (requestedSize <= 9) return 9;
    if (requestedSize <= 12) return 12;
    if (requestedSize <= 16) return 16;
    if (requestedSize <= 20) return 20;
    if (requestedSize <= 24) return 24;
    return requestedSize;
}

void MeasureText(CHFont* font, const char* text, int* width, int* height)
{
    if (!font || !text || !width || !height)
        return;
    
    *width = static_cast<int>(strlen(text)) * font->nRealSize;
    *height = font->nRealSize;
}

} // namespace CHFontInternal