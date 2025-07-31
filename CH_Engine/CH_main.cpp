#include "CH_main.h"
#include "CH_texture.h"
#include "CH_datafile.h"
#include "CH_sprite.h"

// Version string for file compatibility checking
const char CH_VERSION[64] = "MAXFILE CH 00001";

// Global DirectX 11 objects
CH_CORE_DLL_API CHComPtr<ID3D11Device> g_D3DDevice;
CH_CORE_DLL_API CHComPtr<ID3D11DeviceContext> g_D3DContext;
CH_CORE_DLL_API CHComPtr<IDXGISwapChain> g_SwapChain;
CH_CORE_DLL_API CHComPtr<ID3D11RenderTargetView> g_RenderTargetView;
CH_CORE_DLL_API CHComPtr<ID3D11DepthStencilView> g_DepthStencilView;
CH_CORE_DLL_API CHComPtr<ID3D11Texture2D> g_DepthStencilBuffer;

// Global state variables (maintaining exact same structure as original)
CH_CORE_DLL_API CHDisplayMode g_DisplayMode;
CH_CORE_DLL_API HWND g_hWnd;
CH_CORE_DLL_API CHViewport g_Viewport;
CH_CORE_DLL_API XMMATRIX g_ViewMatrix;
CH_CORE_DLL_API XMMATRIX g_ProjectMatrix;
CH_CORE_DLL_API CHPresentParameters g_Present;
CH_CORE_DLL_API D3D_FEATURE_LEVEL g_FeatureLevel;
CH_CORE_DLL_API CRITICAL_SECTION g_CriticalSection;

// Internal render state management
CHInternal::RenderStateManager CHInternal::g_RenderStateManager;
CHInternal::CompatibilityShaderManager CHInternal::g_ShaderManager;

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
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPLICATION));
    wc.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = lpTitle;
    RegisterClass(&wc);

    DEVMODE mode;
    mode.dmSize = sizeof(mode);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);

    HWND hWnd = CreateWindow(lpTitle,
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
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

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
    g_Viewport.X = 0;
    g_Viewport.Y = 0;
    g_Viewport.Width = dwWidth;
    g_Viewport.Height = dwHeight;
    g_Viewport.MinZ = 0.0f;
    g_Viewport.MaxZ = 1.0f;

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = static_cast<float>(g_Viewport.X);
    viewport.TopLeftY = static_cast<float>(g_Viewport.Y);
    viewport.Width = static_cast<float>(g_Viewport.Width);
    viewport.Height = static_cast<float>(g_Viewport.Height);
    viewport.MinDepth = g_Viewport.MinZ;
    viewport.MaxDepth = g_Viewport.MaxZ;
    
    g_D3DContext->RSSetViewports(1, &viewport);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Initialize compatibility shaders
    if (FAILED(CHInternal::g_ShaderManager.Initialize()))
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
    CHInternal::g_ShaderManager.Cleanup();
    CHSpriteInternal::g_SpriteShaderManager.Cleanup();
    CHInternal::g_RenderStateManager.Reset();
    
    g_DepthStencilView.Reset();
    g_DepthStencilBuffer.Reset();
    g_RenderTargetView.Reset();
    g_SwapChain.Reset();
    g_D3DContext.Reset();
    g_D3DDevice.Reset();
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

// Render state manager implementation
namespace CHInternal {

void RenderStateManager::SetRenderState(CHRenderStateType state, DWORD value)
{
    // Cache state changes and apply them in batches for performance
    if (m_currentState.currentStates[static_cast<int>(state)] == value)
        return;

    m_currentState.currentStates[static_cast<int>(state)] = value;
    
    // Apply immediately for critical states, batch others
    switch (state)
    {
        case CH_RS_CULLMODE:
        case CH_RS_ZFUNC:
        case CH_RS_ZENABLE:
        case CH_RS_ZWRITEENABLE:
            ApplyStates();
            break;
        default:
            // Batch other states
            break;
    }
}

void RenderStateManager::SetTextureStageState(DWORD stage, CHTextureStageStateType type, DWORD value)
{
    if (stage >= 8)
        return;

    if (m_currentState.currentTextureStates[stage][static_cast<int>(type)] == value)
        return;

    m_currentState.currentTextureStates[stage][static_cast<int>(type)] = value;
    
    // Apply sampler state changes immediately
    ApplyStates();
}

void RenderStateManager::ApplyStates()
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
    rasterizerDesc.AntialiasedLineEnable = m_currentState.currentStates[CH_RS_EDGEANTIALIAS] ? TRUE : FALSE;

    // Cache and create rasterizer state
    size_t hash = 0;
    // Simple hash of rasterizer desc
    hash ^= std::hash<UINT>{}(rasterizerDesc.CullMode);
    hash ^= std::hash<BOOL>{}(rasterizerDesc.FrontCounterClockwise);
    hash ^= std::hash<BOOL>{}(rasterizerDesc.MultisampleEnable);

    auto it = m_rasterizerCache.find(hash);
    if (it == m_rasterizerCache.end())
    {
        CHComPtr<ID3D11RasterizerState> rasterizerState;
        if (SUCCEEDED(g_D3DDevice->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf())))
        {
            m_rasterizerCache[hash] = rasterizerState;
            g_D3DContext->RSSetState(rasterizerState.Get());
        }
    }
    else
    {
        g_D3DContext->RSSetState(it->second.Get());
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

    depthStencilDesc.StencilEnable = FALSE;

    // Cache and create depth stencil state
    hash = 0;
    hash ^= std::hash<BOOL>{}(depthStencilDesc.DepthEnable);
    hash ^= std::hash<UINT>{}(depthStencilDesc.DepthWriteMask);
    hash ^= std::hash<UINT>{}(depthStencilDesc.DepthFunc);

    auto dsIt = m_depthStencilCache.find(hash);
    if (dsIt == m_depthStencilCache.end())
    {
        CHComPtr<ID3D11DepthStencilState> depthStencilState;
        if (SUCCEEDED(g_D3DDevice->CreateDepthStencilState(&depthStencilDesc, depthStencilState.GetAddressOf())))
        {
            m_depthStencilCache[hash] = depthStencilState;
            g_D3DContext->OMSetDepthStencilState(depthStencilState.Get(), 0);
        }
    }
    else
    {
        g_D3DContext->OMSetDepthStencilState(dsIt->second.Get(), 0);
    }
}

void RenderStateManager::Reset()
{
    m_rasterizerCache.clear();
    m_depthStencilCache.clear();
    m_blendCache.clear();
    m_samplerCache.clear();
    ZeroMemory(&m_currentState, sizeof(m_currentState));
}

// Shader manager implementation
HRESULT CompatibilityShaderManager::Initialize()
{
    // Create basic vertex shader for compatibility
    const char* vertexShaderSource = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix World;
            matrix View;
            matrix Projection;
        };
        
        struct VS_INPUT
        {
            float4 Pos : POSITION;
            float4 Color : COLOR;
            float2 Tex : TEXCOORD0;
        };
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float4 Color : COLOR;
            float2 Tex : TEXCOORD0;
        };
        
        PS_INPUT main(VS_INPUT input)
        {
            PS_INPUT output = (PS_INPUT)0;
            output.Pos = mul(input.Pos, World);
            output.Pos = mul(output.Pos, View);
            output.Pos = mul(output.Pos, Projection);
            output.Color = input.Color;
            output.Tex = input.Tex;
            return output;
        }
    )";

    const char* pixelShaderSource = R"(
        Texture2D shaderTexture : register(t0);
        SamplerState sampleType : register(s0);
        
        struct PS_INPUT
        {
            float4 Pos : SV_POSITION;
            float4 Color : COLOR;
            float2 Tex : TEXCOORD0;
        };
        
        float4 main(PS_INPUT input) : SV_TARGET
        {
            float4 textureColor = shaderTexture.Sample(sampleType, input.Tex);
            return textureColor * input.Color;
        }
    )";

    // Compile and create shaders
    CHComPtr<ID3DBlob> vertexShaderBlob;
    CHComPtr<ID3DBlob> errorBlob;
    
    HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
                           "main", "vs_4_0", 0, 0, vertexShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());
    
    if (FAILED(hr))
        return hr;

    hr = g_D3DDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), 
                                        vertexShaderBlob->GetBufferSize(), 
                                        nullptr, m_defaultVertexShader.GetAddressOf());
    if (FAILED(hr))
        return hr;

    CHComPtr<ID3DBlob> pixelShaderBlob;
    hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr,
                   "main", "ps_4_0", 0, 0, pixelShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());
    
    if (FAILED(hr))
        return hr;

    hr = g_D3DDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), 
                                       pixelShaderBlob->GetBufferSize(), 
                                       nullptr, m_defaultPixelShader.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
    return hr;
}

void CompatibilityShaderManager::SetDefaultShaders()
{
    g_D3DContext->VSSetShader(m_defaultVertexShader.Get(), nullptr, 0);
    g_D3DContext->PSSetShader(m_defaultPixelShader.Get(), nullptr, 0);
    g_D3DContext->IASetInputLayout(m_defaultInputLayout.Get());
    g_D3DContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
}

void CompatibilityShaderManager::UpdateConstantBuffer(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& proj)
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

void CompatibilityShaderManager::Cleanup()
{
    m_constantBuffer.Reset();
    m_defaultInputLayout.Reset();
    m_defaultPixelShader.Reset();
    m_defaultVertexShader.Reset();
}

} // namespace CHInternal