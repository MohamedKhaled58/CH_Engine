#ifndef _CH_main_h_
#define _CH_main_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <algorithm>
#include "CH_common.h"

// DirectX 11 device and context management
extern CH_CORE_DLL_API CHComPtr<ID3D11Device> g_D3DDevice;
extern CH_CORE_DLL_API CHComPtr<ID3D11DeviceContext> g_D3DContext;
extern CH_CORE_DLL_API CHComPtr<IDXGISwapChain> g_SwapChain;
extern CH_CORE_DLL_API CHComPtr<ID3D11RenderTargetView> g_RenderTargetView;
extern CH_CORE_DLL_API CHComPtr<ID3D11DepthStencilView> g_DepthStencilView;
extern CH_CORE_DLL_API CHComPtr<ID3D11Texture2D> g_DepthStencilBuffer;

// Version string for file compatibility checking
extern CH_CORE_DLL_API const char CH_VERSION[64];

// Compatibility structures and globals (maintaining exact same interface)
struct CHDisplayMode {
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    DXGI_FORMAT Format;
};

struct CHPresentParameters {
    BOOL Windowed;
    UINT BackBufferCount;
    UINT BackBufferWidth;
    UINT BackBufferHeight;
    HWND hDeviceWindow;
    DXGI_SWAP_EFFECT SwapEffect;
    DXGI_FORMAT BackBufferFormat;
    BOOL EnableAutoDepthStencil;
    DXGI_FORMAT AutoDepthStencilFormat;
};

struct CHViewport {
    UINT X;
    UINT Y;
    UINT Width;
    UINT Height;
    FLOAT MinZ;
    FLOAT MaxZ;
};

// Global state (maintaining exact same variables as original)
extern CH_CORE_DLL_API CHDisplayMode g_DisplayMode;
extern CH_CORE_DLL_API HWND g_hWnd;
extern CH_CORE_DLL_API CHViewport g_Viewport;
extern CH_CORE_DLL_API XMMATRIX g_ViewMatrix;
extern CH_CORE_DLL_API XMMATRIX g_ProjectMatrix;
extern CH_CORE_DLL_API CHPresentParameters g_Present;

// DirectX 11 feature level and device capabilities
extern CH_CORE_DLL_API D3D_FEATURE_LEVEL g_FeatureLevel;

// Thread safety
extern CH_CORE_DLL_API CRITICAL_SECTION g_CriticalSection;

// Exact same API as original engine
// Return values:
// 1   = Success
// 0   = DX version error
// -1  = Hardware not supported
// -2  = 16-bit mode not supported  
// -3  = Alpha mode not supported
CH_CORE_DLL_API
int Init3D(HINSTANCE hInst,
          const char* lpTitle,
          DWORD dwWidth,
          DWORD dwHeight,
          BOOL bWindowed,
          WNDPROC proc,
          DWORD dwBackCount);

CH_CORE_DLL_API
int Init3DEx(HWND hWnd,
            DWORD dwWidth,
            DWORD dwHeight,
            BOOL bWindowed,
            DWORD dwBackCount);

CH_CORE_DLL_API
void Quit3D();

CH_CORE_DLL_API
BOOL Begin3D();

CH_CORE_DLL_API
BOOL End3D();

CH_CORE_DLL_API
int IfDeviceLost();

CH_CORE_DLL_API
BOOL ResetDevice();

CH_CORE_DLL_API
BOOL ClearBuffer(BOOL bZBuffer, BOOL bTarget, DWORD color);

CH_CORE_DLL_API
BOOL Flip();

// DirectX 8 style render state management (translated to DX11 internally)
enum CHRenderStateType {
    CH_RS_AMBIENT = 1,
    CH_RS_LIGHTING = 2,
    CH_RS_CULLMODE = 3,
    CH_RS_ZFUNC = 4,
    CH_RS_EDGEANTIALIAS = 5,
    CH_RS_MULTISAMPLEANTIALIAS = 6,
    CH_RS_ALPHABLENDENABLE = 7,
    CH_RS_SRCBLEND = 8,
    CH_RS_DESTBLEND = 9,
    CH_RS_ZENABLE = 10,
    CH_RS_ZWRITEENABLE = 11
};

enum CHTextureStageStateType {
    CH_TSS_MINFILTER = 1,
    CH_TSS_MAGFILTER = 2,
    CH_TSS_MIPFILTER = 3,
    CH_TSS_COLOROP = 4,
    CH_TSS_COLORARG1 = 5,
    CH_TSS_COLORARG2 = 6,
    CH_TSS_ALPHAOP = 7,
    CH_TSS_ALPHAARG1 = 8,
    CH_TSS_ALPHAARG2 = 9
};

enum CHCullMode {
    CH_CULL_NONE = 1,
    CH_CULL_CW = 2,
    CH_CULL_CCW = 3
};

enum CHCompareFunc {
    CH_CMP_NEVER = 1,
    CH_CMP_LESS = 2,
    CH_CMP_EQUAL = 3,
    CH_CMP_LESSEQUAL = 4,
    CH_CMP_GREATER = 5,
    CH_CMP_NOTEQUAL = 6,
    CH_CMP_GREATEREQUAL = 7,
    CH_CMP_ALWAYS = 8
};

enum CHBlend {
    CH_BLEND_ZERO = 1,
    CH_BLEND_ONE = 2,
    CH_BLEND_SRCCOLOR = 3,
    CH_BLEND_INVSRCCOLOR = 4,
    CH_BLEND_SRCALPHA = 5,
    CH_BLEND_INVSRCALPHA = 6,
    CH_BLEND_DESTALPHA = 7,
    CH_BLEND_INVDESTALPHA = 8,
    CH_BLEND_DESTCOLOR = 9,
    CH_BLEND_INVDESTCOLOR = 10
};

enum CHTextureFilter {
    CH_TEXF_NONE = 0,
    CH_TEXF_POINT = 1,
    CH_TEXF_LINEAR = 2,
    CH_TEXF_ANISOTROPIC = 3
};

CH_CORE_DLL_API
void SetRenderState(CHRenderStateType state, DWORD dwValue);

CH_CORE_DLL_API
void SetTextureStageState(DWORD dwStage,
                         CHTextureStageStateType type,
                         DWORD dwValue);

CH_CORE_DLL_API
BOOL SetTexture(DWORD dwStage, ID3D11ShaderResourceView* lpTex);

// Frame rate calculation functions (maintaining exact same interface)
CH_CORE_DLL_API
DWORD CalcRate();

CH_CORE_DLL_API
BOOL LimitRate(DWORD dwRate);

// Internal DirectX 11 specific functionality
namespace CHInternal {
    // Render state management
    class RenderStateManager {
    private:
        struct RenderState {
            CHComPtr<ID3D11RasterizerState> rasterizerState;
            CHComPtr<ID3D11DepthStencilState> depthStencilState;
            CHComPtr<ID3D11BlendState> blendState;
            CHComPtr<ID3D11SamplerState> samplerStates[8];
            DWORD currentStates[32];
            DWORD currentTextureStates[8][16];
        };
        
        RenderState m_currentState;
        CHComPtr<ID3D11RasterizerState> m_rasterizerStates[16];
        CHComPtr<ID3D11DepthStencilState> m_depthStencilStates[16];
        CHComPtr<ID3D11BlendState> m_blendStates[16];
        CHComPtr<ID3D11SamplerState> m_samplerStates[8][16];
        
    public:
        void SetRenderState(CHRenderStateType state, DWORD value);
        void SetTextureStageState(DWORD stage, CHTextureStageStateType type, DWORD value);
        void ApplyStates();
        void Reset();
    };
    
    extern RenderStateManager g_RenderStateManager;
    
    // Shader management for compatibility
    class CompatibilityShaderManager {
    private:
        CHComPtr<ID3D11VertexShader> m_defaultVertexShader;
        CHComPtr<ID3D11PixelShader> m_defaultPixelShader;
        CHComPtr<ID3D11InputLayout> m_defaultInputLayout;
        CHComPtr<ID3D11Buffer> m_constantBuffer;
        
    public:
        BOOL Initialize();
        void SetDefaultShaders();
        void UpdateConstantBuffer(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj);
        void Cleanup();
    };
    
    extern CompatibilityShaderManager g_ShaderManager;
}

#endif // _CH_main_h_