#ifndef _CH_main_h_
#define _CH_main_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include <windows.h>
#include <winerror.h>  // For HRESULT definition
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <algorithm>
#include <ole2.h>  // For HRESULT definition
#include "CH_common.h"

// Include all constants
#include "CH_constants.h" // Or paste the constants directly here

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
extern CH_CORE_DLL_API D3D11_VIEWPORT g_Viewport;  // Changed from CHViewport to D3D11_VIEWPORT
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
            DWORD currentStates[512];          // Increased size for all states
            DWORD currentTextureStates[8][64]; // Increased size for all texture states
        };

        RenderState m_currentState;

        // State conversion helpers
        D3D11_BLEND ConvertBlendMode(DWORD chBlend);
        D3D11_STENCIL_OP ConvertStencilOp(DWORD chStencilOp);
        D3D11_COMPARISON_FUNC ConvertStencilFunc(DWORD chFunc);
        D3D11_TEXTURE_ADDRESS_MODE ConvertTextureAddress(DWORD chAddress);
        void ApplySamplerState(DWORD stage);

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
        CHComPtr<ID3D11PixelShader> m_simplePixelShader;
        CHComPtr<ID3D11InputLayout> m_defaultInputLayout;
        CHComPtr<ID3D11Buffer> m_constantBuffer;
        CHComPtr<ID3D11SamplerState> m_defaultSampler;

    public:
        HRESULT Initialize();
        void SetDefaultShaders();
        void SetLightmapShaders();
        void UpdateConstantBuffer(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj);
        void Cleanup();
    };

    // DirectX 8 compatibility typedefs
    typedef CHDisplayMode D3DDISPLAYMODE;
    typedef CHDisplayMode CH_D3DDISPLAYMODE;

    typedef CHViewport D3DVIEWPORT8;
    typedef CHViewport CH_D3DVIEWPORT8;

    typedef CHPresentParameters D3DPRESENT_PARAMETERS;
    typedef CHPresentParameters CH_D3DPRESENT_PARAMETERS;

    // DirectX 8 capabilities structure (simplified for compatibility)
    struct CH_D3DCAPS8 {
        DWORD DeviceType;
        UINT AdapterOrdinal;
        DWORD Caps;
        DWORD Caps2;
        DWORD Caps3;
        DWORD PresentationIntervals;
        DWORD CursorCaps;
        DWORD DevCaps;
        DWORD PrimitiveMiscCaps;
        DWORD RasterCaps;
        DWORD ZCmpCaps;
        DWORD SrcBlendCaps;
        DWORD DestBlendCaps;
        DWORD AlphaCmpCaps;
        DWORD ShadeCaps;
        DWORD TextureCaps;
        DWORD TextureFilterCaps;
        DWORD CubeTextureFilterCaps;
        DWORD VolumeTextureFilterCaps;
        DWORD TextureAddressCaps;
        DWORD VolumeTextureAddressCaps;
        DWORD LineCaps;
        DWORD MaxTextureWidth;
        DWORD MaxTextureHeight;
        DWORD MaxVolumeExtent;
        DWORD MaxTextureRepeat;
        DWORD MaxTextureAspectRatio;
        DWORD MaxAnisotropy;
        float MaxVertexW;
        float GuardBandLeft;
        float GuardBandTop;
        float GuardBandRight;
        float GuardBandBottom;
        float ExtentsAdjust;
        DWORD StencilCaps;
        DWORD FVFCaps;
        DWORD TextureOpCaps;
        DWORD MaxTextureBlendStages;
        DWORD MaxSimultaneousTextures;
        DWORD VertexProcessingCaps;
        DWORD MaxActiveLights;
        DWORD MaxUserClipPlanes;
        DWORD MaxVertexBlendMatrices;
        DWORD MaxVertexBlendMatrixIndex;
        float MaxPointSize;
        DWORD MaxPrimitiveCount;
        DWORD MaxVertexIndex;
        DWORD MaxStreams;
        DWORD MaxStreamStride;
        DWORD VertexShaderVersion;
        DWORD MaxVertexShaderConst;
        DWORD PixelShaderVersion;
        float MaxPixelShaderValue;
    };

    typedef CH_D3DCAPS8 D3DCAPS8;

    // Global variables for internal use
    extern RenderStateManager g_RenderStateManager;
    extern CompatibilityShaderManager g_CompatibilityShaderManager;
}

// Physics internal namespace
namespace CHPhyInternal {
    class PhyShaderManager;
    extern PhyShaderManager g_PhyShaderManager;
}

#endif // _CH_main_h_

