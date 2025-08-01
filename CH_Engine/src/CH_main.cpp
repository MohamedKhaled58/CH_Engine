#include "CH_main.h"
#include "CH_texture.h"
#include "CH_datafile.h"
#include "CH_sprite.h"
#include "CH_phy.h"
#include <windows.h>
#include <winuser.h>
#include <winres.h>
#include <stringapiset.h>
#include <winerror.h>
#include <ole2.h>

// Version string for file compatibility checking
const char CH_VERSION[64] = "MAXFILE CH 00001";

// Global DirectX 11 objects (equivalent to DirectX 8 globals)
CH_CORE_DLL_API CHComPtr<ID3D11Device> g_D3DDevice;
CH_CORE_DLL_API CHComPtr<ID3D11DeviceContext> g_D3DContext;
CH_CORE_DLL_API CHComPtr<IDXGISwapChain> g_SwapChain;
CH_CORE_DLL_API CHComPtr<ID3D11RenderTargetView> g_RenderTargetView;
CH_CORE_DLL_API CHComPtr<ID3D11DepthStencilView> g_DepthStencilView;
CH_CORE_DLL_API CHComPtr<ID3D11DepthStencilState> g_DepthStencilState;
CH_CORE_DLL_API CHComPtr<ID3D11RasterizerState> g_RasterizerState;
CH_CORE_DLL_API CHComPtr<ID3D11BlendState> g_BlendState;
CH_CORE_DLL_API CHComPtr<ID3D11Texture2D> g_DepthStencilBuffer;
CH_CORE_DLL_API D3D_FEATURE_LEVEL g_FeatureLevel;

// Global state variables (maintaining exact same structure as original)
CH_CORE_DLL_API CHInternal::CH_D3DDISPLAYMODE g_DisplayMode;
CH_CORE_DLL_API HWND g_hWnd;
CH_CORE_DLL_API D3D11_VIEWPORT g_Viewport;
CH_CORE_DLL_API XMMATRIX g_ViewMatrix;
CH_CORE_DLL_API XMMATRIX g_ProjectMatrix;
CH_CORE_DLL_API DXGI_SWAP_CHAIN_DESC g_SwapChainDesc;
CH_CORE_DLL_API CHInternal::CH_D3DCAPS8 g_D3DCaps;
CH_CORE_DLL_API CHInternal::CH_D3DVIEWPORT8 g_Viewport8;
CH_CORE_DLL_API CHInternal::CH_D3DPRESENT_PARAMETERS g_Present;
CH_CORE_DLL_API CRITICAL_SECTION g_CriticalSection;

// Internal render state management
namespace CHInternal {
    RenderStateManager g_RenderStateManager;
    CompatibilityShaderManager g_CompatibilityShaderManager;
}

// Physics internal management

BOOL APIENTRY DllMain(HINSTANCE hModule, 
                     DWORD ul_reason_for_call, 
                     LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

int Init3D(HINSTANCE hInst,
          const char* lpTitle,
          DWORD dwWidth,
          DWORD dwHeight,
          BOOL bWindowed,
          WNDPROC proc,
          DWORD dwBackCount)
{
    // Create window class (maintaining exact same behavior as original)
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    
    // Convert const char* to LPCWSTR for Unicode compatibility
    int wideLength = MultiByteToWideChar(CP_ACP, 0, lpTitle, -1, NULL, 0);
    wchar_t* wideTitle = new wchar_t[wideLength];
    MultiByteToWideChar(CP_ACP, 0, lpTitle, -1, wideTitle, wideLength);
    wc.lpszClassName = wideTitle;
    RegisterClass(&wc);

    DEVMODE mode;
    mode.dmSize = sizeof(mode);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);

    HWND hWnd = CreateWindowA(lpTitle,
                            lpTitle,
                            WS_POPUP,
                            (mode.dmPelsWidth - dwWidth) / 2,
                            (mode.dmPelsHeight - dwHeight) / 2,
                            dwWidth,
                            dwHeight,
                            GetDesktopWindow(),
                            NULL,
                            hInst,
                            NULL);
    if (!hWnd)
        return 0;

    return Init3DEx(hWnd, dwWidth, dwHeight, bWindowed, dwBackCount);
}

int Init3DEx(HWND hWnd,
            DWORD dwWidth,
            DWORD dwHeight,
            BOOL bWindowed,
            DWORD dwBackCount)
{
    g_DisplayMode.Width = dwWidth;
    g_DisplayMode.Height = dwHeight;
    g_DisplayMode.RefreshRate = 60;

    // Create DirectX 11 device and swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = dwBackCount;
    swapChainDesc.BufferDesc.Width = dwWidth;
    swapChainDesc.BufferDesc.Height = dwHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Start with 32-bit
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = bWindowed ? TRUE : FALSE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Try hardware acceleration first, fallback to software
    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE
    };
    UINT numDriverTypes = 3;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    UINT numFeatureLevels = 4;

    HRESULT hr;
    for (UINT i = 0; i < numDriverTypes; i++)
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            driverTypes[i],
            nullptr,
            D3D11_CREATE_DEVICE_DEBUG,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            g_SwapChain.GetAddressOf(),
            g_D3DDevice.GetAddressOf(),
            &g_FeatureLevel,
            g_D3DContext.GetAddressOf()
        );

        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
    {
        // Try with 16-bit color formats for compatibility
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B5G6R5_UNORM;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            featureLevels,
            numFeatureLevels,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            g_SwapChain.GetAddressOf(),
            g_D3DDevice.GetAddressOf(),
            &g_FeatureLevel,
            g_D3DContext.GetAddressOf()
        );

        if (FAILED(hr))
        {
            // Final fallback to any supported format
            swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                0,
                featureLevels,
                numFeatureLevels,
                D3D11_SDK_VERSION,
                &swapChainDesc,
                g_SwapChain.GetAddressOf(),
                g_D3DDevice.GetAddressOf(),
                &g_FeatureLevel,
                g_D3DContext.GetAddressOf()
            );

            if (FAILED(hr))
                return -1; // Hardware not supported
        }
    }

    g_DisplayMode.Format = swapChainDesc.BufferDesc.Format;

    // Create render target view
    CHComPtr<ID3D11Texture2D> backBuffer;
    hr = g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), 
                               reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    if (FAILED(hr))
        return -1;

    hr = g_D3DDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, 
                                           g_RenderTargetView.GetAddressOf());
    if (FAILED(hr))
        return -1;

    // Create depth stencil buffer and view
    D3D11_TEXTURE2D_DESC depthStencilDesc = {};
    depthStencilDesc.Width = dwWidth;
    depthStencilDesc.Height = dwHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = g_D3DDevice->CreateTexture2D(&depthStencilDesc, nullptr, 
                                     g_DepthStencilBuffer.GetAddressOf());
    if (FAILED(hr))
        return -1;

    hr = g_D3DDevice->CreateDepthStencilView(g_DepthStencilBuffer.Get(), nullptr, 
                                           g_DepthStencilView.GetAddressOf());
    if (FAILED(hr))
        return -1;

    // Set render targets
    g_D3DContext->OMSetRenderTargets(1, g_RenderTargetView.GetAddressOf(), 
                                    g_DepthStencilView.Get());

    // Set viewport
    g_Viewport.TopLeftX = 0.0f;
    g_Viewport.TopLeftY = 0.0f;
    g_Viewport.Width = static_cast<float>(dwWidth);
    g_Viewport.Height = static_cast<float>(dwHeight);
    g_Viewport.MinDepth = 0.0f;
    g_Viewport.MaxDepth = 1.0f;

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = g_Viewport.TopLeftX;
    viewport.TopLeftY = g_Viewport.TopLeftY;
    viewport.Width = g_Viewport.Width;
    viewport.Height = g_Viewport.Height;
    viewport.MinDepth = g_Viewport.MinDepth;
    viewport.MaxDepth = g_Viewport.MaxDepth;
    
    g_D3DContext->RSSetViewports(1, &viewport);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Initialize compatibility shaders
    if (FAILED(CHInternal::g_CompatibilityShaderManager.Initialize()))
        return -1;
    
    // Initialize sprite shaders
    if (FAILED(CHSpriteInternal::g_SpriteShaderManager.Initialize()))
        return -1;

    // Set default render states (maintaining exact same defaults as original)
    SetRenderState(CH_RS_AMBIENT, 0xFFFFFFFF);
    SetRenderState(CH_RS_LIGHTING, TRUE);
    SetRenderState(CH_RS_CULLMODE, CH_CULL_CW);
    SetRenderState(CH_RS_ZFUNC, CH_CMP_LESSEQUAL);
    SetRenderState(CH_RS_EDGEANTIALIAS, TRUE);
    SetRenderState(CH_RS_MULTISAMPLEANTIALIAS, TRUE);
    SetTextureStageState(0, CH_TSS_MINFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(0, CH_TSS_MAGFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(0, CH_TSS_MIPFILTER, CH_TEXF_LINEAR);

    // Initialize texture system
    for (int t = 0; t < TEX_MAX; t++)
        g_lpTex[t] = nullptr;

    // Initialize critical section for thread safety
    InitializeCriticalSection(&g_CriticalSection);

    g_hWnd = hWnd;

    // Store present parameters for compatibility
    g_Present.Windowed = bWindowed;
    g_Present.BackBufferCount = dwBackCount;
    g_Present.BackBufferWidth = g_DisplayMode.Width;
    g_Present.BackBufferHeight = g_DisplayMode.Height;
    g_Present.hDeviceWindow = g_hWnd;
    g_Present.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    g_Present.BackBufferFormat = g_DisplayMode.Format;
    g_Present.EnableAutoDepthStencil = TRUE;
    g_Present.AutoDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    return 1; // Success
}

CH_CORE_DLL_API
void Quit3D()
{
    CHInternal::g_CompatibilityShaderManager.Cleanup();
    CHSpriteInternal::g_SpriteShaderManager.Cleanup();
    CHInternal::g_RenderStateManager.Reset();
    
    g_DepthStencilView.Reset();
    g_DepthStencilBuffer.Reset();
    g_RenderTargetView.Reset();
    g_SwapChain.Reset();
    g_D3DContext.Reset();
    g_D3DDevice.Reset();
    
    // Cleanup critical section
    DeleteCriticalSection(&g_CriticalSection);
}

CH_CORE_DLL_API
BOOL Begin3D()
{
    // DirectX 11 doesn't require explicit BeginScene/EndScene
    // but we maintain the API for compatibility
    return TRUE;
}

CH_CORE_DLL_API
BOOL End3D()
{
    // DirectX 11 doesn't require explicit BeginScene/EndScene
    // but we maintain the API for compatibility
    return TRUE;
}

CH_CORE_DLL_API
int IfDeviceLost()
{
    // DirectX 11 handles device lost differently
    // For compatibility, we return 0 (no device lost)
    return 0;
}

CH_CORE_DLL_API
BOOL ResetDevice()
{
    // DirectX 11 handles device reset differently
    // For compatibility, we just reapply render states
    SetRenderState(CH_RS_AMBIENT, 0xFFFFFFFF);
    return TRUE;
}

CH_CORE_DLL_API
BOOL ClearBuffer(BOOL bZBuffer, BOOL bTarget, DWORD color)
{
    // Convert DWORD color to float array
    float clearColor[4];
    clearColor[0] = ((color >> 16) & 0xFF) / 255.0f; // Red
    clearColor[1] = ((color >> 8) & 0xFF) / 255.0f;  // Green
    clearColor[2] = (color & 0xFF) / 255.0f;         // Blue
    clearColor[3] = ((color >> 24) & 0xFF) / 255.0f; // Alpha

    if (bTarget && g_RenderTargetView)
    {
        g_D3DContext->ClearRenderTargetView(g_RenderTargetView.Get(), clearColor);
    }

    if (bZBuffer && g_DepthStencilView)
    {
        g_D3DContext->ClearDepthStencilView(g_DepthStencilView.Get(), 
                                          D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
                                          1.0f, 0);
    }

    return TRUE;
}

CH_CORE_DLL_API
BOOL Flip()
{
    if (!g_SwapChain)
        return FALSE;

    HRESULT hr = g_SwapChain->Present(0, 0);
    return SUCCEEDED(hr);
}

CH_CORE_DLL_API
void SetRenderState(CHRenderStateType state, DWORD dwValue)
{
    CHInternal::g_RenderStateManager.SetRenderState(state, dwValue);
}

CH_CORE_DLL_API
void SetTextureStageState(DWORD dwStage, CHTextureStageStateType type, DWORD dwValue)
{
    CHInternal::g_RenderStateManager.SetTextureStageState(dwStage, type, dwValue);
}

CH_CORE_DLL_API
BOOL SetTexture(DWORD dwStage, ID3D11ShaderResourceView* lpTex)
{
    if (dwStage >= 8)
        return FALSE;

    g_D3DContext->PSSetShaderResources(dwStage, 1, &lpTex);
    return TRUE;
}

// Frame rate calculation (maintaining exact same behavior)
CH_CORE_DLL_API
DWORD CalcRate()
{
    static DWORD countcur = 0, countold = 0;
    static DWORD timecur = 0, timeold = 0;
    static DWORD rate = 0;

    countcur++;
    timecur = timeGetTime();

    if (timecur - timeold > 1000)
    {
        rate = (countcur - countold) * 1000 / (timecur - timeold);
        timeold = timecur;
        countold = countcur;
    }
    return rate;
}

CH_CORE_DLL_API
BOOL LimitRate(DWORD dwRate)
{
    static DWORD dwFrame = 0;
    static DWORD dwFrameOld = 0;
    dwFrame = timeGetTime();
    if (dwFrame - dwFrameOld >= dwRate)
    {
        dwFrameOld = dwFrame;
        return TRUE;
    }
    return FALSE;
}

// Add this to CH_main.cpp - Complete shader implementation

namespace CHInternal {

HRESULT CHInternal::CompatibilityShaderManager::Initialize()
    {
        // Create vertex shader for scene rendering
        const char* vertexShaderSource = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        };
        
        struct VS_INPUT
        {
            float3 Pos : POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
            float2 LightTex : TEXCOORD1;
        };
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
            float2 LightTex : TEXCOORD1;
            float3 WorldPos : TEXCOORD2;
        };
        
        PS_INPUT main(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            
            // Transform position to world space
            float4 worldPos = mul(float4(input.Pos, 1.0f), World);
            
            // Transform to view space
            float4 viewPos = mul(worldPos, View);
            
            // Transform to projection space
            output.Pos = mul(viewPos, Projection);
            
            // Transform normal to world space
            output.Normal = normalize(mul(input.Normal, (float3x3)World));
            
            // Pass through texture coordinates
            output.Tex = input.Tex;
            output.LightTex = input.LightTex;
            output.WorldPos = worldPos.xyz;
            
            return output;
        }
    )";

        const char* pixelShaderSource = R"(
        Texture2D diffuseTexture : register(t0);
        Texture2D lightmapTexture : register(t1);
        SamplerState sampleType : register(s0);
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
            float2 LightTex : TEXCOORD1;
            float3 WorldPos : TEXCOORD2;
        };
        
        float4 main(PS_INPUT input) : SV_TARGET
        {
            // Sample diffuse texture
            float4 diffuseColor = diffuseTexture.Sample(sampleType, input.Tex);
            
            // Sample lightmap texture
            float4 lightmapColor = lightmapTexture.Sample(sampleType, input.LightTex);
            
            // Combine diffuse and lightmap (modulate)
            float4 finalColor = diffuseColor * lightmapColor;
            
            return finalColor;
        }
    )";

        // Create simple pixel shader without lightmap
        const char* simplePixelShaderSource = R"(
        Texture2D diffuseTexture : register(t0);
        SamplerState sampleType : register(s0);
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
            float2 LightTex : TEXCOORD1;
            float3 WorldPos : TEXCOORD2;
        };
        
        float4 main(PS_INPUT input) : SV_TARGET
        {
            // Sample diffuse texture only
            float4 diffuseColor = diffuseTexture.Sample(sampleType, input.Tex);
            
            // Simple lighting calculation
            float3 lightDir = normalize(float3(0.0f, 0.0f, -1.0f));
            float ndotl = max(0.0f, dot(normalize(input.Normal), lightDir));
            
            // Apply basic lighting
            diffuseColor.rgb *= (0.3f + 0.7f * ndotl); // Ambient + diffuse
            
            return diffuseColor;
        }
    )";

        // Compile vertex shader
        CHComPtr<ID3DBlob> vertexShaderBlob;
        CHComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
            "main", "vs_4_0", 0, 0, vertexShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                char* errorMsg = static_cast<char*>(errorBlob->GetBufferPointer());
                // ErrorMessage(errorMsg); // Commented out as ErrorMessage might not be defined
            }
            return hr;
        }

        hr = g_D3DDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            nullptr, m_defaultVertexShader.GetAddressOf());
        if (FAILED(hr))
            return hr;

        // Compile lightmap pixel shader
        CHComPtr<ID3DBlob> pixelShaderBlob;
        hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr,
            "main", "ps_4_0", 0, 0, pixelShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                char* errorMsg = static_cast<char*>(errorBlob->GetBufferPointer());
                // ErrorMessage(errorMsg);
            }
            return hr;
        }

        hr = g_D3DDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            nullptr, m_defaultPixelShader.GetAddressOf());
        if (FAILED(hr))
            return hr;

        // Compile simple pixel shader
        CHComPtr<ID3DBlob> simplePixelShaderBlob;
        hr = D3DCompile(simplePixelShaderSource, strlen(simplePixelShaderSource), nullptr, nullptr, nullptr,
            "main", "ps_4_0", 0, 0, simplePixelShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

        if (FAILED(hr))
            return hr;

        hr = g_D3DDevice->CreatePixelShader(simplePixelShaderBlob->GetBufferPointer(),
            simplePixelShaderBlob->GetBufferSize(),
            nullptr, m_simplePixelShader.GetAddressOf());
        if (FAILED(hr))
            return hr;

        // Create input layout for scene vertices
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = g_D3DDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            m_defaultInputLayout.GetAddressOf());
        if (FAILED(hr))
            return hr;

        // Create constant buffer
        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.ByteWidth = sizeof(XMMATRIX) * 3; // World, View, Projection
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = g_D3DDevice->CreateBuffer(&constantBufferDesc, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr))
            return hr;

        // Create default sampler state
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = 0;
        samplerDesc.BorderColor[1] = 0;
        samplerDesc.BorderColor[2] = 0;
        samplerDesc.BorderColor[3] = 0;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        hr = g_D3DDevice->CreateSamplerState(&samplerDesc, m_defaultSampler.GetAddressOf());
        return hr;
    }

void CHInternal::CompatibilityShaderManager::SetDefaultShaders()
    {
        g_D3DContext->VSSetShader(m_defaultVertexShader.Get(), nullptr, 0);
        g_D3DContext->PSSetShader(m_simplePixelShader.Get(), nullptr, 0); // Use simple by default
        g_D3DContext->IASetInputLayout(m_defaultInputLayout.Get());
        g_D3DContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        // Set default sampler
        g_D3DContext->PSSetSamplers(0, 1, m_defaultSampler.GetAddressOf());
    }

void CHInternal::CompatibilityShaderManager::SetLightmapShaders()
    {
        g_D3DContext->VSSetShader(m_defaultVertexShader.Get(), nullptr, 0);
        g_D3DContext->PSSetShader(m_defaultPixelShader.Get(), nullptr, 0); // Use lightmap shader
        g_D3DContext->IASetInputLayout(m_defaultInputLayout.Get());
        g_D3DContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        // Set default sampler for both texture stages
        g_D3DContext->PSSetSamplers(0, 1, m_defaultSampler.GetAddressOf());
    }

void CHInternal::CompatibilityShaderManager::UpdateConstantBuffer(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = g_D3DContext->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            XMMATRIX* matrices = reinterpret_cast<XMMATRIX*>(mappedResource.pData);
            matrices[0] = XMMatrixTranspose(world);
            matrices[1] = XMMatrixTranspose(view);
            matrices[2] = XMMatrixTranspose(proj);
            g_D3DContext->Unmap(m_constantBuffer.Get(), 0);
        }
    }

void CHInternal::CompatibilityShaderManager::Cleanup()
    {
        m_defaultSampler.Reset();
        m_constantBuffer.Reset();
        m_defaultInputLayout.Reset();
        m_simplePixelShader.Reset();
        m_defaultPixelShader.Reset();
        m_defaultVertexShader.Reset();
    }


}

// CHInternal namespace method implementations
void CHInternal::RenderStateManager::SetRenderState(CHRenderStateType state, DWORD value)
{
    // Bounds check
    if (static_cast<int>(state) >= 512)
        return;

    // Cache state changes and apply them efficiently
    if (m_currentState.currentStates[static_cast<int>(state)] == value)
        return;

    m_currentState.currentStates[static_cast<int>(state)] = value;

    // Apply immediately for critical states
    switch (state)
    {
    case CH_RS_CULLMODE:
    case CH_RS_ZFUNC:
    case CH_RS_ZENABLE:
    case CH_RS_ZWRITEENABLE:
    case CH_RS_ALPHABLENDENABLE:
    case CH_RS_SRCBLEND:
    case CH_RS_DESTBLEND:
    case CH_RS_ALPHATESTENABLE:
    case CH_RS_ALPHAREF:
    case CH_RS_ALPHAFUNC:
        ApplyStates();
        break;
    default:
        // Other states can be batched
        break;
    }
}

void CHInternal::RenderStateManager::SetTextureStageState(DWORD stage, CHTextureStageStateType type, DWORD value)
{
    if (stage >= 8 || static_cast<int>(type) >= 64)
        return;

    if (m_currentState.currentTextureStates[stage][static_cast<int>(type)] == value)
        return;

    m_currentState.currentTextureStates[stage][static_cast<int>(type)] = value;

    // Apply sampler state changes for filtering
    if (type == CH_TSS_MINFILTER || type == CH_TSS_MAGFILTER || type == CH_TSS_MIPFILTER)
    {
        ApplySamplerState(stage);
    }
}

void CHInternal::RenderStateManager::ApplyStates()
        {
            // Apply rasterizer state
            D3D11_RASTERIZER_DESC rasterizerDesc = {};
            rasterizerDesc.FillMode = D3D11_FILL_SOLID;
            rasterizerDesc.CullMode = D3D11_CULL_BACK;

            DWORD cullMode = m_currentState.currentStates[CH_RS_CULLMODE];
            switch (cullMode)
            {
            case CH_CULL_NONE:
                rasterizerDesc.CullMode = D3D11_CULL_NONE;
                break;
            case CH_CULL_CW:
                rasterizerDesc.CullMode = D3D11_CULL_BACK;
                rasterizerDesc.FrontCounterClockwise = FALSE;
                break;
            case CH_CULL_CCW:
                rasterizerDesc.CullMode = D3D11_CULL_BACK;
                rasterizerDesc.FrontCounterClockwise = TRUE;
                break;
            }

            rasterizerDesc.DepthBias = 0;
            rasterizerDesc.DepthBiasClamp = 0.0f;
            rasterizerDesc.SlopeScaledDepthBias = 0.0f;
            rasterizerDesc.DepthClipEnable = TRUE;
            rasterizerDesc.ScissorEnable = FALSE;
            rasterizerDesc.MultisampleEnable = m_currentState.currentStates[CH_RS_MULTISAMPLEANTIALIAS] ? TRUE : FALSE;
            rasterizerDesc.AntialiasedLineEnable = m_currentState.currentStates[CH_RS_ANTIALIASEDLINEENABLE] ? TRUE : FALSE;

            // Create and set rasterizer state
            CHComPtr<ID3D11RasterizerState> rasterizerState;
            if (SUCCEEDED(g_D3DDevice->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf())))
            {
                g_D3DContext->RSSetState(rasterizerState.Get());
            }

            // Apply depth stencil state
            D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
            depthStencilDesc.DepthEnable = m_currentState.currentStates[CH_RS_ZENABLE] ? TRUE : FALSE;
            depthStencilDesc.DepthWriteMask = m_currentState.currentStates[CH_RS_ZWRITEENABLE] ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;

            DWORD zFunc = m_currentState.currentStates[CH_RS_ZFUNC];
            switch (zFunc)
            {
            case CH_CMP_NEVER:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
                break;
            case CH_CMP_LESS:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
                break;
            case CH_CMP_EQUAL:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
                break;
            case CH_CMP_LESSEQUAL:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
                break;
            case CH_CMP_GREATER:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
                break;
            case CH_CMP_NOTEQUAL:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
                break;
            case CH_CMP_GREATEREQUAL:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
                break;
            case CH_CMP_ALWAYS:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
                break;
            default:
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
                break;
            }

            // Handle stencil operations if enabled
            depthStencilDesc.StencilEnable = m_currentState.currentStates[CH_RS_STENCILENABLE] ? TRUE : FALSE;
            if (depthStencilDesc.StencilEnable)
            {
                depthStencilDesc.StencilReadMask = static_cast<UINT8>(m_currentState.currentStates[CH_RS_STENCILMASK]);
                depthStencilDesc.StencilWriteMask = static_cast<UINT8>(m_currentState.currentStates[CH_RS_STENCILWRITEMASK]);

                // Front face stencil operations
                depthStencilDesc.FrontFace.StencilFailOp = ConvertStencilOp(m_currentState.currentStates[CH_RS_STENCILFAIL]);
                depthStencilDesc.FrontFace.StencilDepthFailOp = ConvertStencilOp(m_currentState.currentStates[CH_RS_STENCILZFAIL]);
                depthStencilDesc.FrontFace.StencilPassOp = ConvertStencilOp(m_currentState.currentStates[CH_RS_STENCILPASS]);
                depthStencilDesc.FrontFace.StencilFunc = ConvertStencilFunc(m_currentState.currentStates[CH_RS_STENCILFUNC]);

                // Back face stencil operations (same as front for now)
                depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
            }

            // Create and set depth stencil state
            CHComPtr<ID3D11DepthStencilState> depthStencilState;
            if (SUCCEEDED(g_D3DDevice->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf())))
            {
                UINT stencilRef = m_currentState.currentStates[CH_RS_STENCILREF];
                g_D3DContext->OMSetDepthStencilState(depthStencilState.Get(), stencilRef);
            }

            // Apply blend state
            if (m_currentState.currentStates[CH_RS_ALPHABLENDENABLE])
            {
                D3D11_BLEND_DESC blendDesc = {};
                blendDesc.AlphaToCoverageEnable = FALSE;
                blendDesc.IndependentBlendEnable = FALSE;
                blendDesc.RenderTarget[0].BlendEnable = TRUE;

                // Convert CH blend modes to D3D11
                DWORD srcBlend = m_currentState.currentStates[CH_RS_SRCBLEND];
                DWORD destBlend = m_currentState.currentStates[CH_RS_DESTBLEND];

                blendDesc.RenderTarget[0].SrcBlend = ConvertBlendMode(srcBlend);
                blendDesc.RenderTarget[0].DestBlend = ConvertBlendMode(destBlend);
                blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
                blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

                CHComPtr<ID3D11BlendState> blendState;
                if (SUCCEEDED(g_D3DDevice->CreateBlendState(&blendDesc, blendState.GetAddressOf())))
                {
                    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                    g_D3DContext->OMSetBlendState(blendState.Get(), blendFactor, 0xffffffff);
                }
            }
            else
            {
                // Disable blending
                g_D3DContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
            }
        }

D3D11_BLEND CHInternal::RenderStateManager::ConvertBlendMode(DWORD chBlend)
        {
            switch (chBlend)
            {
            case CH_BLEND_ZERO:
                return D3D11_BLEND_ZERO;
            case CH_BLEND_ONE:
                return D3D11_BLEND_ONE;
            case CH_BLEND_SRCCOLOR:
                return D3D11_BLEND_SRC_COLOR;
            case CH_BLEND_INVSRCCOLOR:
                return D3D11_BLEND_INV_SRC_COLOR;
            case CH_BLEND_SRCALPHA:
                return D3D11_BLEND_SRC_ALPHA;
            case CH_BLEND_INVSRCALPHA:
                return D3D11_BLEND_INV_SRC_ALPHA;
            case CH_BLEND_DESTALPHA:
                return D3D11_BLEND_DEST_ALPHA;
            case CH_BLEND_INVDESTALPHA:
                return D3D11_BLEND_INV_DEST_ALPHA;
            case CH_BLEND_DESTCOLOR:
                return D3D11_BLEND_DEST_COLOR;
            case CH_BLEND_INVDESTCOLOR:
                return D3D11_BLEND_INV_DEST_COLOR;
            case CH_BLEND_SRCALPHASAT:
                return D3D11_BLEND_SRC_ALPHA_SAT;
            default:
                return D3D11_BLEND_ONE;
            }
        }

D3D11_STENCIL_OP CHInternal::RenderStateManager::ConvertStencilOp(DWORD chStencilOp)
        {
            switch (chStencilOp)
            {
            case CH_STENCILOP_KEEP:
                return D3D11_STENCIL_OP_KEEP;
            case CH_STENCILOP_ZERO:
                return D3D11_STENCIL_OP_ZERO;
            case CH_STENCILOP_REPLACE:
                return D3D11_STENCIL_OP_REPLACE;
            case CH_STENCILOP_INCRSAT:
                return D3D11_STENCIL_OP_INCR_SAT;
            case CH_STENCILOP_DECRSAT:
                return D3D11_STENCIL_OP_DECR_SAT;
            case CH_STENCILOP_INVERT:
                return D3D11_STENCIL_OP_INVERT;
            case CH_STENCILOP_INCR:
                return D3D11_STENCIL_OP_INCR;
            case CH_STENCILOP_DECR:
                return D3D11_STENCIL_OP_DECR;
            default:
                return D3D11_STENCIL_OP_KEEP;
            }
        }

D3D11_COMPARISON_FUNC CHInternal::RenderStateManager::ConvertStencilFunc(DWORD chFunc)
        {
            switch (chFunc)
            {
            case CH_CMP_NEVER:
                return D3D11_COMPARISON_NEVER;
            case CH_CMP_LESS:
                return D3D11_COMPARISON_LESS;
            case CH_CMP_EQUAL:
                return D3D11_COMPARISON_EQUAL;
            case CH_CMP_LESSEQUAL:
                return D3D11_COMPARISON_LESS_EQUAL;
            case CH_CMP_GREATER:
                return D3D11_COMPARISON_GREATER;
            case CH_CMP_NOTEQUAL:
                return D3D11_COMPARISON_NOT_EQUAL;
            case CH_CMP_GREATEREQUAL:
                return D3D11_COMPARISON_GREATER_EQUAL;
            case CH_CMP_ALWAYS:
                return D3D11_COMPARISON_ALWAYS;
            default:
                return D3D11_COMPARISON_ALWAYS;
            }
        }

void CHInternal::RenderStateManager::ApplySamplerState(DWORD stage)
        {
            if (stage >= 8)
                return;

            D3D11_SAMPLER_DESC samplerDesc = {};

            // Get filter settings
            DWORD minFilter = m_currentState.currentTextureStates[stage][CH_TSS_MINFILTER];
            DWORD magFilter = m_currentState.currentTextureStates[stage][CH_TSS_MAGFILTER];
            DWORD mipFilter = m_currentState.currentTextureStates[stage][CH_TSS_MIPFILTER];

            // Convert to D3D11 filter
            if (minFilter == CH_TEXF_POINT && magFilter == CH_TEXF_POINT)
            {
                if (mipFilter == CH_TEXF_POINT)
                    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                else if (mipFilter == CH_TEXF_LINEAR)
                    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                else
                    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
            else if (minFilter == CH_TEXF_LINEAR && magFilter == CH_TEXF_LINEAR)
            {
                if (mipFilter == CH_TEXF_POINT)
                    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                else if (mipFilter == CH_TEXF_LINEAR)
                    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                else
                    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            }
            else if (minFilter == CH_TEXF_ANISOTROPIC || magFilter == CH_TEXF_ANISOTROPIC)
            {
                samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            }
            else
            {
                samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Default
            }

            // Address modes
            DWORD addressU = m_currentState.currentTextureStates[stage][CH_TSS_ADDRESSU];
            DWORD addressV = m_currentState.currentTextureStates[stage][CH_TSS_ADDRESSV];
            DWORD addressW = m_currentState.currentTextureStates[stage][CH_TSS_ADDRESSW];

            samplerDesc.AddressU = ConvertTextureAddress(addressU);
            samplerDesc.AddressV = ConvertTextureAddress(addressV);
            samplerDesc.AddressW = ConvertTextureAddress(addressW);

            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = m_currentState.currentTextureStates[stage][CH_TSS_MAXANISOTROPY];
            if (samplerDesc.MaxAnisotropy == 0)
                samplerDesc.MaxAnisotropy = 1;

            samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

            // Border color
            DWORD borderColor = m_currentState.currentTextureStates[stage][CH_TSS_BORDERCOLOR];
            samplerDesc.BorderColor[0] = ((borderColor >> 16) & 0xFF) / 255.0f; // R
            samplerDesc.BorderColor[1] = ((borderColor >> 8) & 0xFF) / 255.0f;  // G
            samplerDesc.BorderColor[2] = (borderColor & 0xFF) / 255.0f;         // B
            samplerDesc.BorderColor[3] = ((borderColor >> 24) & 0xFF) / 255.0f; // A

            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

            CHComPtr<ID3D11SamplerState> samplerState;
            if (SUCCEEDED(g_D3DDevice->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf())))
            {
                g_D3DContext->PSSetSamplers(stage, 1, samplerState.GetAddressOf());
            }
        }

D3D11_TEXTURE_ADDRESS_MODE CHInternal::RenderStateManager::ConvertTextureAddress(DWORD chAddress)
        {
            switch (chAddress)
            {
            case CH_TADDRESS_WRAP:
                return D3D11_TEXTURE_ADDRESS_WRAP;
            case CH_TADDRESS_MIRROR:
                return D3D11_TEXTURE_ADDRESS_MIRROR;
            case CH_TADDRESS_CLAMP:
                return D3D11_TEXTURE_ADDRESS_CLAMP;
            case CH_TADDRESS_BORDER:
                return D3D11_TEXTURE_ADDRESS_BORDER;
            case CH_TADDRESS_MIRRORONCE:
                return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
            default:
                return D3D11_TEXTURE_ADDRESS_WRAP;
            }
        }
    
// CHPhyInternal namespace implementation
namespace CHPhyInternal {

BOOL CHPhyInternal::PhyShaderManager::Initialize()
    {
        // Create vertex shader for skeletal animation
        const char* vertexShaderSource = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        };
        
        cbuffer BoneMatrices : register(b1)
        {
            matrix BoneMatrices[64]; // Support up to 64 bones
        };
        
        struct VS_INPUT
        {
            float3 Pos : POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
            float4 BoneIndices : BLENDINDICES;
            float4 BoneWeights : BLENDWEIGHT;
        };
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
        };
        
        PS_INPUT main(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            
            // Apply bone transformations
            float4 skinnedPos = float4(0, 0, 0, 0);
            float3 skinnedNormal = float3(0, 0, 0);
            
            for (int i = 0; i < 4; i++)
            {
                if (input.BoneWeights[i] > 0.0f)
                {
                    int boneIndex = (int)input.BoneIndices[i];
                    if (boneIndex >= 0 && boneIndex < 64)
                    {
                        float4x4 boneMatrix = BoneMatrices[boneIndex];
                        skinnedPos += mul(float4(input.Pos, 1.0f), boneMatrix) * input.BoneWeights[i];
                        skinnedNormal += mul(input.Normal, (float3x3)boneMatrix) * input.BoneWeights[i];
                    }
                }
            }
            
            // Transform to world space
            float4 worldPos = mul(skinnedPos, World);
            
            // Transform to view space
            float4 viewPos = mul(worldPos, View);
            
            // Transform to projection space
            output.Pos = mul(viewPos, Projection);
            
            // Transform normal to world space
            output.Normal = normalize(mul(skinnedNormal, (float3x3)World));
            
            // Pass through texture coordinates
            output.Tex = input.Tex;
            
            return output;
        }
    )";

        const char* pixelShaderSource = R"(
        Texture2D diffuseTexture : register(t0);
        SamplerState sampleType : register(s0);
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float3 Normal : NORMAL;
            float2 Tex : TEXCOORD0;
        };
        
        float4 main(PS_INPUT input) : SV_TARGET
        {
            // Sample diffuse texture
            float4 diffuseColor = diffuseTexture.Sample(sampleType, input.Tex);
            
            // Simple lighting calculation
            float3 lightDir = normalize(float3(0.0f, 0.0f, -1.0f));
            float ndotl = max(0.0f, dot(normalize(input.Normal), lightDir));
            
            // Apply basic lighting
            diffuseColor.rgb *= (0.3f + 0.7f * ndotl); // Ambient + diffuse
            
            return diffuseColor;
        }
    )";

        // Compile vertex shader
        CHComPtr<ID3DBlob> vertexShaderBlob;
        CHComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
            "main", "vs_4_0", 0, 0, vertexShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

        if (FAILED(hr))
        {
            return FALSE;
        }

        hr = g_D3DDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            nullptr, m_skeletalVertexShader.GetAddressOf());
        if (FAILED(hr))
            return FALSE;

        // Compile pixel shader
        CHComPtr<ID3DBlob> pixelShaderBlob;
        hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr,
            "main", "ps_4_0", 0, 0, pixelShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

        if (FAILED(hr))
            return FALSE;

        hr = g_D3DDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            nullptr, m_skeletalPixelShader.GetAddressOf());
        if (FAILED(hr))
            return FALSE;

        // Create input layout for skeletal vertices
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = g_D3DDevice->CreateInputLayout(layout, 5,
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            m_skeletalInputLayout.GetAddressOf());
        if (FAILED(hr))
            return FALSE;

        // Create bone matrix constant buffer
        D3D11_BUFFER_DESC boneBufferDesc = {};
        boneBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        boneBufferDesc.ByteWidth = sizeof(XMMATRIX) * 64; // 64 bones max
        boneBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        boneBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = g_D3DDevice->CreateBuffer(&boneBufferDesc, nullptr, m_boneMatrixBuffer.GetAddressOf());
        if (FAILED(hr))
            return FALSE;

        return TRUE;
    }

void CHPhyInternal::PhyShaderManager::SetSkeletalShaders()
    {
        g_D3DContext->VSSetShader(m_skeletalVertexShader.Get(), nullptr, 0);
        g_D3DContext->PSSetShader(m_skeletalPixelShader.Get(), nullptr, 0);
        g_D3DContext->IASetInputLayout(m_skeletalInputLayout.Get());
        g_D3DContext->VSSetConstantBuffers(1, 1, m_boneMatrixBuffer.GetAddressOf()); // Bone matrices at slot 1
    }

void CHPhyInternal::PhyShaderManager::SetNormalShaders()
    {
        // Use the same shaders as skeletal for now
        SetSkeletalShaders();
    }

void CHPhyInternal::PhyShaderManager::SetAlphaShaders()
    {
        // Use the same shaders as skeletal for now
        SetSkeletalShaders();
    }

void CHPhyInternal::PhyShaderManager::UpdateBoneMatrices(const XMMATRIX* boneMatrices, UINT boneCount)
    {
        if (boneCount > 64) boneCount = 64; // Limit to 64 bones

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = g_D3DContext->Map(m_boneMatrixBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            XMMATRIX* matrices = reinterpret_cast<XMMATRIX*>(mappedResource.pData);
            for (UINT i = 0; i < boneCount; i++)
            {
                matrices[i] = XMMatrixTranspose(boneMatrices[i]);
            }
            g_D3DContext->Unmap(m_boneMatrixBuffer.Get(), 0);
        }
    }

void CHPhyInternal::PhyShaderManager::Cleanup()
    {
        m_boneMatrixBuffer.Reset();
        m_skeletalInputLayout.Reset();
        m_skeletalPixelShader.Reset();
        m_skeletalVertexShader.Reset();
    }

} // namespace CHPhyInternal

void CHInternal::RenderStateManager::Reset()
    {
        ZeroMemory(&m_currentState, sizeof(m_currentState));

        // Set default states (matching DirectX 8 defaults)
        m_currentState.currentStates[CH_RS_ZENABLE] = TRUE;
        m_currentState.currentStates[CH_RS_ZWRITEENABLE] = TRUE;
        m_currentState.currentStates[CH_RS_CULLMODE] = CH_CULL_CCW;
        m_currentState.currentStates[CH_RS_ALPHABLENDENABLE] = FALSE;
        m_currentState.currentStates[CH_RS_LIGHTING] = TRUE;
        m_currentState.currentStates[CH_RS_AMBIENT] = 0x00000000;
        m_currentState.currentStates[CH_RS_SRCBLEND] = CH_BLEND_ONE;
        m_currentState.currentStates[CH_RS_DESTBLEND] = CH_BLEND_ZERO;
        m_currentState.currentStates[CH_RS_ZFUNC] = CH_CMP_LESSEQUAL;
        m_currentState.currentStates[CH_RS_ALPHATESTENABLE] = FALSE;
        m_currentState.currentStates[CH_RS_ALPHAREF] = 0;
        m_currentState.currentStates[CH_RS_ALPHAFUNC] = CH_CMP_ALWAYS;
        m_currentState.currentStates[CH_RS_DITHERENABLE] = FALSE;
        m_currentState.currentStates[CH_RS_FOGENABLE] = FALSE;
        m_currentState.currentStates[CH_RS_SPECULARENABLE] = FALSE;
        m_currentState.currentStates[CH_RS_STENCILENABLE] = FALSE;
        m_currentState.currentStates[CH_RS_STENCILMASK] = 0xFFFFFFFF;
        m_currentState.currentStates[CH_RS_STENCILWRITEMASK] = 0xFFFFFFFF;
        m_currentState.currentStates[CH_RS_STENCILREF] = 0;
        m_currentState.currentStates[CH_RS_MULTISAMPLEANTIALIAS] = TRUE;
        m_currentState.currentStates[CH_RS_ANTIALIASEDLINEENABLE] = FALSE;

        // Set default texture stage states
        for (int i = 0; i < 8; i++)
        {
            m_currentState.currentTextureStates[i][CH_TSS_MINFILTER] = CH_TEXF_POINT;
            m_currentState.currentTextureStates[i][CH_TSS_MAGFILTER] = CH_TEXF_POINT;
            m_currentState.currentTextureStates[i][CH_TSS_MIPFILTER] = CH_TEXF_NONE;
            m_currentState.currentTextureStates[i][CH_TSS_COLOROP] = (i == 0) ? CH_TOP_MODULATE : CH_TOP_DISABLE;
            m_currentState.currentTextureStates[i][CH_TSS_COLORARG1] = CH_TA_TEXTURE;
            m_currentState.currentTextureStates[i][CH_TSS_COLORARG2] = CH_TA_DIFFUSE;
            m_currentState.currentTextureStates[i][CH_TSS_ALPHAOP] = (i == 0) ? CH_TOP_SELECTARG1 : CH_TOP_DISABLE;
            m_currentState.currentTextureStates[i][CH_TSS_ALPHAARG1] = CH_TA_TEXTURE;
            m_currentState.currentTextureStates[i][CH_TSS_ALPHAARG2] = CH_TA_DIFFUSE;
            m_currentState.currentTextureStates[i][CH_TSS_ADDRESSU] = CH_TADDRESS_WRAP;
            m_currentState.currentTextureStates[i][CH_TSS_ADDRESSV] = CH_TADDRESS_WRAP;
            m_currentState.currentTextureStates[i][CH_TSS_ADDRESSW] = CH_TADDRESS_WRAP;
            m_currentState.currentTextureStates[i][CH_TSS_BORDERCOLOR] = 0x00000000;
            m_currentState.currentTextureStates[i][CH_TSS_MAXANISOTROPY] = 1;
            m_currentState.currentTextureStates[i][CH_TSS_MAXMIPLEVEL] = 0;
            m_currentState.currentTextureStates[i][CH_TSS_MIPMAPLODBIAS] = 0;
        }
    }

