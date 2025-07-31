#include "CH_sprite.h"
#include "CH_main.h"

// Global sprite shader manager
CHSpriteInternal::SpriteShaderManager CHSpriteInternal::g_SpriteShaderManager;

// Staging texture for lock/unlock operations
static CHComPtr<ID3D11Texture2D> g_StagingTexture;
static D3D11_MAPPED_SUBRESOURCE g_MappedResource = {};
static bool g_IsTextureLocked = false;

CH_CORE_DLL_API
void Sprite_Clear(CHSprite* lpSprite)
{
    for (int v = 0; v < 4; v++)
    {
        ZeroMemory(&lpSprite->vertex[v], sizeof(CHSpriteVertex));
        lpSprite->vertex[v].rhw = 1.0f;
        lpSprite->vertex[v].color = 0xFFFFFFFF; // D3DCOLOR_ARGB(255, 255, 255, 255)
    }
    
    // Set default UV coordinates
    lpSprite->vertex[0].u = 0.0f;
    lpSprite->vertex[0].v = 0.0f;

    lpSprite->vertex[1].u = 0.0f;
    lpSprite->vertex[1].v = 1.0f;

    lpSprite->vertex[2].u = 1.0f;
    lpSprite->vertex[2].v = 0.0f;

    lpSprite->vertex[3].u = 1.0f;
    lpSprite->vertex[3].v = 1.0f;

    lpSprite->lpTex = nullptr;
}

CH_CORE_DLL_API
BOOL Sprite_Load(CHSprite** lpSprite,
                char* lpName,
                CHPool pool,
                BOOL bDuplicate,
                DWORD colorkey)
{
    *lpSprite = new CHSprite;
    Sprite_Clear(*lpSprite);

    if (Texture_Load(&(*lpSprite)->lpTex, lpName, 1, pool, bDuplicate, colorkey) == -1)
    {
        delete *lpSprite;
        *lpSprite = nullptr;
        return FALSE;
    }

    Sprite_SetCoor(*lpSprite, nullptr, 0, 0);
    return TRUE;
}

CH_CORE_DLL_API
BOOL Sprite_Create(CHSprite** lpSprite,
                  DWORD dwWidth,
                  DWORD dwHeight,
                  DWORD dwMipLevels,
                  CHFormat format,
                  CHPool pool)
{
    *lpSprite = new CHSprite;
    Sprite_Clear(*lpSprite);

    if (!Texture_Create(&(*lpSprite)->lpTex, dwWidth, dwHeight, dwMipLevels, format, pool))
    {
        delete *lpSprite;
        *lpSprite = nullptr;
        return FALSE;
    }
    return TRUE;
}

CH_CORE_DLL_API
void Sprite_Unload(CHSprite** lpSprite)
{
    if (!lpSprite || !*lpSprite)
        return;
        
    if ((*lpSprite)->lpTex)
        Texture_Unload(&(*lpSprite)->lpTex);
    
    delete *lpSprite;
    *lpSprite = nullptr;
}

CH_CORE_DLL_API
void Sprite_Mirror(CHSprite* lpSprite)
{
    float u, v;

    // Swap top vertices UV
    u = lpSprite->vertex[0].u;
    v = lpSprite->vertex[0].v;
    lpSprite->vertex[0].u = lpSprite->vertex[2].u;
    lpSprite->vertex[0].v = lpSprite->vertex[2].v;
    lpSprite->vertex[2].u = u;
    lpSprite->vertex[2].v = v;

    // Swap bottom vertices UV
    u = lpSprite->vertex[1].u;
    v = lpSprite->vertex[1].v;
    lpSprite->vertex[1].u = lpSprite->vertex[3].u;
    lpSprite->vertex[1].v = lpSprite->vertex[3].v;
    lpSprite->vertex[3].u = u;
    lpSprite->vertex[3].v = v;
}

CH_CORE_DLL_API
void Sprite_SetCoor(CHSprite* lpSprite,
                   RECT* lpSrc,
                   int nX,
                   int nY,
                   DWORD dwWidth,
                   DWORD dwHeight)
{
    if (!lpSprite || !lpSprite->lpTex)
        return;

    float uu = 1.0f / static_cast<float>(lpSprite->lpTex->Info.Width);
    float vv = 1.0f / static_cast<float>(lpSprite->lpTex->Info.Height);

    uu = 0; // Original code sets these to 0
    vv = 0;

    // Set UV coordinates
    if (!lpSrc)
    {
        // Use full texture
        lpSprite->vertex[0].u = 0.0f + uu;
        lpSprite->vertex[0].v = 0.0f + vv;

        lpSprite->vertex[1].u = 0.0f + uu;
        lpSprite->vertex[1].v = 1.0f - vv;

        lpSprite->vertex[2].u = 1.0f - uu;
        lpSprite->vertex[2].v = 0.0f + vv;

        lpSprite->vertex[3].u = 1.0f - uu;
        lpSprite->vertex[3].v = 1.0f - vv;
    }
    else
    {
        // Use specified rectangle
        lpSprite->vertex[0].u = static_cast<float>(lpSrc->left) / static_cast<float>(lpSprite->lpTex->Info.Width) + uu;
        lpSprite->vertex[0].v = static_cast<float>(lpSrc->top) / static_cast<float>(lpSprite->lpTex->Info.Height) + vv;

        lpSprite->vertex[1].u = lpSprite->vertex[0].u;
        lpSprite->vertex[1].v = static_cast<float>(lpSrc->bottom) / static_cast<float>(lpSprite->lpTex->Info.Height) - vv;

        lpSprite->vertex[2].u = static_cast<float>(lpSrc->right) / static_cast<float>(lpSprite->lpTex->Info.Width) - uu;
        lpSprite->vertex[2].v = lpSprite->vertex[0].v;

        lpSprite->vertex[3].u = lpSprite->vertex[2].u;
        lpSprite->vertex[3].v = lpSprite->vertex[1].v;
    }

    // Set screen coordinates
    DWORD actualWidth = dwWidth == 0 ? lpSprite->lpTex->Info.Width : dwWidth;
    DWORD actualHeight = dwHeight == 0 ? lpSprite->lpTex->Info.Height : dwHeight;

    lpSprite->vertex[0].x = static_cast<float>(nX);
    lpSprite->vertex[0].y = static_cast<float>(nY);

    lpSprite->vertex[1].x = static_cast<float>(nX);
    lpSprite->vertex[1].y = static_cast<float>(nY + actualHeight);

    lpSprite->vertex[2].x = static_cast<float>(nX + actualWidth);
    lpSprite->vertex[2].y = static_cast<float>(nY);

    lpSprite->vertex[3].x = static_cast<float>(nX + actualWidth);
    lpSprite->vertex[3].y = static_cast<float>(nY + actualHeight);
}

CH_CORE_DLL_API
void Sprite_SetColor(CHSprite* lpSprite, BYTE a, BYTE r, BYTE g, BYTE b)
{
    DWORD color = (a << 24) | (r << 16) | (g << 8) | b;
    for (int v = 0; v < 4; v++)
        lpSprite->vertex[v].color = color;
}

CH_CORE_DLL_API
void Sprite_SetVertexColor(CHSprite* lpSprite, DWORD ltColor, DWORD rtColor, DWORD lbColor, DWORD rbColor)
{
    lpSprite->vertex[0].color = ltColor;  // Left top
    lpSprite->vertex[1].color = lbColor;  // Left bottom
    lpSprite->vertex[2].color = rtColor;  // Right top
    lpSprite->vertex[3].color = rbColor;  // Right bottom
}

CH_CORE_DLL_API
void Sprite_Prepare()
{
    CHSpriteInternal::SetupSpriteRenderStates();
}

CH_CORE_DLL_API
BOOL Sprite_Draw(CHSprite* lpSprite, DWORD dwShowWay)
{
    if (!lpSprite || !lpSprite->lpTex)
        return FALSE;

    // Set blend mode based on show way
    DWORD vertexAlpha = lpSprite->vertex[0].color >> 24;
    CHSpriteInternal::SetBlendMode(static_cast<CHSpriteInternal::SpriteBlendMode>(dwShowWay), 
                                  lpSprite->lpTex, vertexAlpha);

    // Set texture
    if (!SetTexture(0, lpSprite->lpTex->lpSRV.Get()))
        return FALSE;

    // Render sprite
    return SUCCEEDED(CHSpriteInternal::RenderSprite(lpSprite));
}

CH_CORE_DLL_API
BOOL Sprite_Draw(CHSprite* lpSpriteUp, CHSprite* lpSpriteDn, 
                UCHAR uAlphaA, UCHAR uAlphaB, UCHAR uAlphaC, UCHAR uAlphaD)
{
    if (!lpSpriteUp || !lpSpriteDn)
        return FALSE;

    return SUCCEEDED(CHSpriteInternal::RenderDualSprite(lpSpriteUp, lpSpriteDn, 
                                                       uAlphaA, uAlphaB, uAlphaC, uAlphaD));
}

CH_CORE_DLL_API
void Sprite_Lock(CHSprite* lpSprite, RECT* lpRect, CHLockedRect* lpReturn)
{
    if (!lpSprite || !lpSprite->lpTex || !lpReturn)
        return;

    // Create staging texture for CPU access
    if (SUCCEEDED(CHSpriteInternal::CreateStagingTexture(lpSprite->lpTex, g_StagingTexture)))
    {
        // Copy from GPU texture to staging
        g_D3DContext->CopyResource(g_StagingTexture.Get(), lpSprite->lpTex->lpTex.Get());

        // Map the staging texture
        HRESULT hr = g_D3DContext->Map(g_StagingTexture.Get(), 0, D3D11_MAP_READ_WRITE, 0, &g_MappedResource);
        if (SUCCEEDED(hr))
        {
            lpReturn->Pitch = g_MappedResource.RowPitch;
            lpReturn->pBits = g_MappedResource.pData;
            g_IsTextureLocked = true;
        }
    }
}

CH_CORE_DLL_API
void Sprite_Unlock(CHSprite* lpSprite)
{
    if (!lpSprite || !lpSprite->lpTex || !g_IsTextureLocked)
        return;

    // Unmap the staging texture
    g_D3DContext->Unmap(g_StagingTexture.Get(), 0);

    // Copy back to GPU texture
    g_D3DContext->CopyResource(lpSprite->lpTex->lpTex.Get(), g_StagingTexture.Get());

    g_IsTextureLocked = false;
    CHSpriteInternal::ReleaseStagingTexture();
}

// Internal implementation
namespace CHSpriteInternal {

void SetupSpriteRenderStates()
{
    SetRenderState(CH_RS_ZENABLE, FALSE);
    SetRenderState(CH_RS_ZWRITEENABLE, FALSE);
    SetRenderState(CH_RS_CULLMODE, CH_CULL_NONE);

    SetTextureStageState(0, CH_TSS_COLORARG1, 1); // D3DTA_TEXTURE
    SetTextureStageState(0, CH_TSS_COLORARG2, 0); // D3DTA_DIFFUSE
    SetTextureStageState(0, CH_TSS_COLOROP, 4);   // D3DTOP_MODULATE

    SetTextureStageState(0, CH_TSS_ALPHAARG1, 1); // D3DTA_TEXTURE
    SetTextureStageState(0, CH_TSS_ALPHAARG2, 0); // D3DTA_DIFFUSE
    SetTextureStageState(0, CH_TSS_ALPHAOP, 4);   // D3DTOP_MODULATE

    SetTextureStageState(0, CH_TSS_MINFILTER, CH_TEXF_POINT);
    SetTextureStageState(0, CH_TSS_MAGFILTER, CH_TEXF_POINT);
    SetTextureStageState(0, CH_TSS_MIPFILTER, CH_TEXF_NONE);

    SetTextureStageState(1, CH_TSS_COLOROP, 1);   // D3DTOP_DISABLE
    SetTextureStageState(1, CH_TSS_ALPHAOP, 1);   // D3DTOP_DISABLE
}

void SetBlendMode(SpriteBlendMode mode, CHTexture* texture, DWORD vertexAlpha)
{
    bool hasAlpha = (texture->Info.Format == CH_FMT_A8R8G8B8 ||
                     texture->Info.Format == CH_FMT_A1R5G5B5 ||
                     texture->Info.Format == CH_FMT_A4R4G4B4 ||
                     texture->Info.Format == CH_FMT_DXT3 ||
                     vertexAlpha < 255);

    switch (mode)
    {
        case BLEND_NORMAL:
            if (hasAlpha)
            {
                SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
                SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCALPHA);
                SetRenderState(CH_RS_DESTBLEND, CH_BLEND_INVSRCALPHA);
            }
            else
            {
                SetRenderState(CH_RS_ALPHABLENDENABLE, FALSE);
            }
            break;

        case BLEND_ADDITIVE:
            if (hasAlpha)
            {
                SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
                SetRenderState(CH_RS_SRCBLEND, CH_BLEND_ONE);
                SetRenderState(CH_RS_DESTBLEND, CH_BLEND_ONE);
            }
            else
            {
                SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
                SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCCOLOR);
                SetRenderState(CH_RS_DESTBLEND, CH_BLEND_ONE);
            }
            break;

        case BLEND_SPECIAL:
            if (hasAlpha)
            {
                SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
                SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCALPHA);
                SetRenderState(CH_RS_DESTBLEND, CH_BLEND_SRCALPHA);
            }
            else
            {
                SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
                SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCCOLOR);
                SetRenderState(CH_RS_DESTBLEND, CH_BLEND_ONE);
            }
            break;
    }
}

HRESULT RenderSprite(CHSprite* sprite)
{
    if (!sprite)
        return E_INVALIDARG;

    // Create temporary vertex buffer for this draw call
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(CHSpriteVertex) * 4;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    CHComPtr<ID3D11Buffer> vertexBuffer;
    HRESULT hr = g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, vertexBuffer.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // Map and fill vertex buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = g_D3DContext->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
        return hr;

    memcpy(mappedResource.pData, sprite->vertex, sizeof(CHSpriteVertex) * 4);
    g_D3DContext->Unmap(vertexBuffer.Get(), 0);

    // Set vertex buffer and draw
    UINT stride = sizeof(CHSpriteVertex);
    UINT offset = 0;
    ID3D11Buffer* vb = vertexBuffer.Get();
    g_D3DContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Set sprite shaders
    g_SpriteShaderManager.SetSpriteShaders();

    // Draw triangle strip (2 triangles = 4 vertices)
    g_D3DContext->Draw(4, 0);

    return S_OK;
}

HRESULT RenderDualSprite(CHSprite* spriteUp, CHSprite* spriteDn, 
                        UCHAR alphaA, UCHAR alphaB, UCHAR alphaC, UCHAR alphaD)
{
    if (!spriteUp || !spriteDn)
        return E_INVALIDARG;

    // Prepare dual-texture vertices
    CHSpriteVertex2 vertices[4];
    
    // Set alpha values for upper sprite
    DWORD alphaValues[4] = {alphaA, alphaB, alphaC, alphaD};
    
    for (int i = 0; i < 4; i++)
    {
        vertices[i].x = spriteUp->vertex[i].x;
        vertices[i].y = spriteUp->vertex[i].y;
        vertices[i].z = spriteUp->vertex[i].z;
        vertices[i].rhw = spriteUp->vertex[i].rhw;
        
        // Apply alpha to upper sprite color
        DWORD baseColor = spriteUp->vertex[i].color & 0x00FFFFFF;
        vertices[i].color = baseColor | (alphaValues[i] << 24);
        
        vertices[i].u1 = vertices[i].u2 = spriteUp->vertex[i].u;
        vertices[i].v1 = vertices[i].v2 = spriteUp->vertex[i].v;
    }

    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(CHSpriteVertex2) * 4;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    CHComPtr<ID3D11Buffer> vertexBuffer;
    HRESULT hr = g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, vertexBuffer.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // Map and fill vertex buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = g_D3DContext->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr))
        return hr;

    memcpy(mappedResource.pData, vertices, sizeof(CHSpriteVertex2) * 4);
    g_D3DContext->Unmap(vertexBuffer.Get(), 0);

    // Set textures
    SetTexture(0, spriteDn->lpTex->lpSRV.Get());
    SetTexture(1, spriteUp->lpTex->lpSRV.Get());

    // Set render states for dual sprite blending
    SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
    
    // Set texture stage states for dual texture blending
    SetTextureStageState(0, CH_TSS_COLOROP, 2);   // D3DTOP_SELECTARG1
    SetTextureStageState(0, CH_TSS_COLORARG1, 1); // D3DTA_TEXTURE
    SetTextureStageState(0, CH_TSS_ALPHAOP, 1);   // D3DTOP_DISABLE

    SetTextureStageState(1, CH_TSS_COLOROP, 12);  // D3DTOP_BLENDDIFFUSEALPHA equivalent
    SetTextureStageState(1, CH_TSS_COLORARG1, 1); // D3DTA_TEXTURE
    SetTextureStageState(1, CH_TSS_COLORARG2, 2); // D3DTA_CURRENT
    SetTextureStageState(1, CH_TSS_ALPHAOP, 1);   // D3DTOP_DISABLE

    SetTextureStageState(0, CH_TSS_MAGFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(1, CH_TSS_MAGFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(0, CH_TSS_MINFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(1, CH_TSS_MINFILTER, CH_TEXF_LINEAR);

    // Set vertex buffer and draw
    UINT stride = sizeof(CHSpriteVertex2);
    UINT offset = 0;
    ID3D11Buffer* vb = vertexBuffer.Get();
    g_D3DContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Set dual sprite shaders
    g_SpriteShaderManager.SetDualSpriteShaders();

    // Draw
    g_D3DContext->Draw(4, 0);

    // Disable second texture stage
    SetTextureStageState(1, CH_TSS_COLOROP, 1); // D3DTOP_DISABLE

    return S_OK;
}

HRESULT CreateStagingTexture(CHTexture* texture, CHComPtr<ID3D11Texture2D>& stagingTexture)
{
    if (!texture || !texture->lpTex)
        return E_INVALIDARG;

    D3D11_TEXTURE2D_DESC desc = texture->d3dDesc;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    return g_D3DDevice->CreateTexture2D(&desc, nullptr, stagingTexture.GetAddressOf());
}

void ReleaseStagingTexture()
{
    g_StagingTexture.Reset();
    ZeroMemory(&g_MappedResource, sizeof(g_MappedResource));
}

// Sprite shader manager implementation
HRESULT SpriteShaderManager::Initialize()
{
    // Create 2D sprite vertex shader (screen-space transformation)
    const char* vertexShaderSource = R"(
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
            
            // Convert screen coordinates to NDC
            // Input is in screen space, convert to normalized device coordinates
            output.Pos.x = (input.Pos.x / 1024.0f) * 2.0f - 1.0f;  // Assuming 1024 screen width
            output.Pos.y = 1.0f - (input.Pos.y / 768.0f) * 2.0f;   // Assuming 768 screen height (flip Y)
            output.Pos.z = input.Pos.z;
            output.Pos.w = input.Pos.w;
            
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

    // Compile and create shaders (similar to previous shader creation)
    CHComPtr<ID3DBlob> vertexShaderBlob;
    CHComPtr<ID3DBlob> errorBlob;
    
    HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
                           "main", "vs_4_0", 0, 0, vertexShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = g_D3DDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), 
                                        vertexShaderBlob->GetBufferSize(), 
                                        nullptr, m_spriteVertexShader.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create pixel shader
    CHComPtr<ID3DBlob> pixelShaderBlob;
    hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr,
                   "main", "ps_4_0", 0, 0, pixelShaderBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = g_D3DDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), 
                                       pixelShaderBlob->GetBufferSize(), 
                                       nullptr, m_spritePixelShader.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create input layout for sprite
    D3D11_INPUT_ELEMENT_DESC spriteLayout[] = SPRITE_VERTEX_FORMAT_DESC;
    hr = g_D3DDevice->CreateInputLayout(spriteLayout, ARRAYSIZE(spriteLayout), 
                                       vertexShaderBlob->GetBufferPointer(),
                                       vertexShaderBlob->GetBufferSize(), 
                                       m_spriteInputLayout.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // TODO: Create dual sprite shaders and input layout
    // For now, use the same shaders
    m_dualSpritePixelShader = m_spritePixelShader;
    m_dualSpriteInputLayout = m_spriteInputLayout;

    return S_OK;
}

void SpriteShaderManager::SetSpriteShaders()
{
    g_D3DContext->VSSetShader(m_spriteVertexShader.Get(), nullptr, 0);
    g_D3DContext->PSSetShader(m_spritePixelShader.Get(), nullptr, 0);
    g_D3DContext->IASetInputLayout(m_spriteInputLayout.Get());
}

void SpriteShaderManager::SetDualSpriteShaders()
{
    g_D3DContext->VSSetShader(m_spriteVertexShader.Get(), nullptr, 0);
    g_D3DContext->PSSetShader(m_dualSpritePixelShader.Get(), nullptr, 0);
    g_D3DContext->IASetInputLayout(m_dualSpriteInputLayout.Get());
}

void SpriteShaderManager::Cleanup()
{
    m_spriteVertexShader.Reset();
    m_spritePixelShader.Reset();
    m_dualSpritePixelShader.Reset();
    m_spriteInputLayout.Reset();
    m_dualSpriteInputLayout.Reset();
}

} // namespace CHSpriteInternal