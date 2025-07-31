#ifndef _CH_sprite_h_
#define _CH_sprite_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include "CH_texture.h"
#include "CH_main.h"

// Sprite vertex definition (maintaining exact same layout as original)
struct CHSpriteVertex {
    float x, y;         // Screen coordinates
    float z;
    float rhw;
    DWORD color;        // Vertex color
    float u, v;         // Texture coordinates
};

// Sprite structure (maintaining exact same layout as C3Sprite)
struct CHSprite {
    CHSpriteVertex vertex[4];
    CHTexture* lpTex;
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Sprite_Clear(CHSprite* lpSprite);

CH_CORE_DLL_API
BOOL Sprite_Load(CHSprite** lpSprite,
                char* lpName,
                CHPool pool = CH_POOL_MANAGED,
                BOOL bDuplicate = true,
                DWORD colorkey = 0);

CH_CORE_DLL_API
BOOL Sprite_Create(CHSprite** lpSprite,
                  DWORD dwWidth,
                  DWORD dwHeight,
                  DWORD dwMipLevels,
                  CHFormat format,
                  CHPool pool);

CH_CORE_DLL_API
void Sprite_Unload(CHSprite** lpSprite);

CH_CORE_DLL_API
void Sprite_SetCoor(CHSprite* lpSprite,
                   RECT* lpSrc,
                   int nX,
                   int nY,
                   DWORD dwWidth = 0,
                   DWORD dwHeight = 0);

CH_CORE_DLL_API
void Sprite_SetColor(CHSprite* lpSprite, BYTE a, BYTE r, BYTE g, BYTE b);

CH_CORE_DLL_API
void Sprite_SetVertexColor(CHSprite* lpSprite, DWORD ltColor, DWORD rtColor, DWORD lbColor, DWORD rbColor);

CH_CORE_DLL_API
void Sprite_Mirror(CHSprite* lpSprite);

CH_CORE_DLL_API
void Sprite_Prepare();

CH_CORE_DLL_API
BOOL Sprite_Draw(CHSprite* lpSprite, DWORD dwShowWay = 0);

// Locked rect structure for sprite pixel access
struct CHLockedRect {
    INT Pitch;
    void* pBits;
};

CH_CORE_DLL_API
void Sprite_Lock(CHSprite* lpSprite, RECT* lpRect, CHLockedRect* lpReturn);

CH_CORE_DLL_API
void Sprite_Unlock(CHSprite* lpSprite);

// Dual sprite drawing function (for alpha blending between two sprites)
CH_CORE_DLL_API
BOOL Sprite_Draw(CHSprite* lpSpriteUp, CHSprite* lpSpriteDn, UCHAR uAlphaA, UCHAR uAlphaB, UCHAR uAlphaC, UCHAR uAlphaD);

// Internal DirectX 11 implementation helpers
namespace CHSpriteInternal {
    // Sprite rendering modes
    enum SpriteBlendMode {
        BLEND_NORMAL = 0,       // Standard alpha blending
        BLEND_ADDITIVE = 1,     // Additive blending
        BLEND_SPECIAL = 2       // Special alpha blending
    };
    
    // Dual-texture sprite vertex for special blending
    struct CHSpriteVertex2 {
        float x, y;
        float z;
        float rhw;
        DWORD color;
        float u1, v1;
        float u2, v2;
    };
    
    // Render state setup
    void SetupSpriteRenderStates();
    void SetBlendMode(SpriteBlendMode mode, CHTexture* texture, DWORD vertexAlpha);
    
    // Rendering utilities
    BOOL RenderSprite(CHSprite* sprite);
    BOOL RenderDualSprite(CHSprite* spriteUp, CHSprite* spriteDn, 
                            UCHAR alphaA, UCHAR alphaB, UCHAR alphaC, UCHAR alphaD);
    
    // Texture access (for Lock/Unlock simulation)
    BOOL CreateStagingTexture(CHTexture* texture, CHComPtr<ID3D11Texture2D>& stagingTexture);
    void ReleaseStagingTexture();
    
    // Shader management for 2D rendering
    class SpriteShaderManager {
    private:
        CHComPtr<ID3D11VertexShader> m_spriteVertexShader;
        CHComPtr<ID3D11PixelShader> m_spritePixelShader;
        CHComPtr<ID3D11PixelShader> m_dualSpritePixelShader;
        CHComPtr<ID3D11InputLayout> m_spriteInputLayout;
        CHComPtr<ID3D11InputLayout> m_dualSpriteInputLayout;
        
    public:
        BOOL Initialize();
        void SetSpriteShaders();
        void SetDualSpriteShaders();
        void Cleanup();
    };
    
    extern SpriteShaderManager g_SpriteShaderManager;
}

// Compatibility types
typedef CHSprite C3Sprite;
typedef CHSpriteVertex SpriteVertex;
typedef CHLockedRect D3DLOCKED_RECT;

// Vertex format constants for compatibility
#define SPRITE_VERTEX_FORMAT_DESC { \
    "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }

#define DUAL_SPRITE_VERTEX_FORMAT_DESC { \
    "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }

#endif // _CH_sprite_h_