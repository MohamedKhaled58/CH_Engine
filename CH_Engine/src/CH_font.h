#ifndef _CH_font_h_
#define _CH_font_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include "CH_texture.h"

// Font vertex structure for rendering text
struct CHFontVertex {
    float x, y;         // Screen coordinates
    float z;
    float rhw;
    DWORD color;        // Text color
    float u, v;         // Texture coordinates
};

// Character structure (maintains exact same layout as C3Char)
struct CHChar {
    char Char[2];               // Character (supports multibyte)
    CHTexture* lpTex;           // Character texture
    DWORD dwTime;               // Last access time
};

// Font structure (maintains exact same layout as C3Font)
struct CHFont {
    DWORD* lpBuffer;            // Font bitmap buffer
    HDC hDC;                    // Device context for font rendering
    HFONT font;                 // Windows font handle
    HBITMAP bitmap;             // Font bitmap
    char szName[256];           // Font name
    int nSize;                  // Font size
    int nRealSize;              // Actual rendered size

    DWORD dwChars;              // Number of cached characters
    CHChar* lpChar[256][256];   // Character cache (2D array for multibyte support)
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Char_Clear(CHChar* lpChar);

CH_CORE_DLL_API
void Font_Clear(CHFont* lpFont);

CH_CORE_DLL_API
BOOL Font_Create(CHFont** lpFont, const char* lpFontName, int nSize);

CH_CORE_DLL_API
void Font_Release(CHFont** lpFont);

CH_CORE_DLL_API
void Font_Prepare();

CH_CORE_DLL_API
BOOL Font_Draw(CHFont* lpFont,
              float fX,
              float fY,
              DWORD color,
              char* lpChar);

// Internal font management
namespace CHFontInternal {
    // Font texture creation
    BOOL CreateCharacterTexture(CHFont* font, CHChar* character, char ch);
    void CacheCharacter(CHFont* font, char ch);
    CHChar* GetCachedCharacter(CHFont* font, char ch);
    
    // Font rendering setup
    void SetupFontRenderStates();
    BOOL RenderTextQuad(CHFontVertex* vertices, CHTexture* texture);
    
    // Font metrics
    int GetFontRealSize(int requestedSize);
    void MeasureText(CHFont* font, const char* text, int* width, int* height);
}

// Compatibility types
typedef CHFont C3Font;
typedef CHChar C3Char;
typedef CHFontVertex FontVertex;

#endif // _CH_font_h_