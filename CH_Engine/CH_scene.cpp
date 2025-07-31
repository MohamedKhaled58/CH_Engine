#include "CH_scene.h"
#include "CH_main.h"
#include "CH_camera.h"

CH_CORE_DLL_API
void Scene_Clear(CHScene* lpScene)
{
    lpScene->lpName = nullptr;

    lpScene->dwVecCount = 0;
    lpScene->lpVB = nullptr;
    lpScene->dwTriCount = 0;
    lpScene->lpIB = nullptr;

    lpScene->lpTexName = nullptr;
    lpScene->nTex = -1;

    lpScene->lplTexName = nullptr;
    lpScene->nlTex = -1;

    lpScene->dwFrameCount = 0;
    lpScene->lpFrame = nullptr;
    lpScene->nFrame = 0;

    lpScene->matrix = XMMatrixIdentity();

    // DirectX 11 specific initialization
    lpScene->vertexBuffer.Reset();
    lpScene->indexBuffer.Reset();
    lpScene->vertexStride = sizeof(CHSceneVertex);
    lpScene->vertexOffset = 0;
}

CH_CORE_DLL_API
BOOL Scene_Load(CHScene** lpScene,
               char* lpName,
               DWORD dwIndex)
{
    FILE* file = fopen(lpName, "rb");
    if (!file)
        return FALSE;

    *lpScene = new CHScene;
    Scene_Clear(*lpScene);

    ChunkHeader chunk;
    DWORD add = 0;

    while (1)
    {
        if (fread(&chunk, sizeof(ChunkHeader), 1, file) != 1)
        {
            fclose(file);
            delete *lpScene;
            *lpScene = nullptr;
            return FALSE;
        }

        if (feof(file))
        {
            fclose(file);
            delete *lpScene;
            *lpScene = nullptr;
            return FALSE;
        }

        if (chunk.byChunkID[0] == 'S' &&
            chunk.byChunkID[1] == 'C' &&
            chunk.byChunkID[2] == 'E' &&
            chunk.byChunkID[3] == 'N')
        {
            if (add < dwIndex)
            {
                add++;
                fseek(file, chunk.dwChunkSize, SEEK_CUR);
                continue;
            }

            // Load scene name
            DWORD temp;
            fread(&temp, sizeof(DWORD), 1, file);
            (*lpScene)->lpName = new char[temp + 1];
            fread((*lpScene)->lpName, 1, temp, file);
            (*lpScene)->lpName[temp] = '\0';

            // Load vertex count
            fread(&(*lpScene)->dwVecCount, sizeof(DWORD), 1, file);
            // Load vertex data
            (*lpScene)->lpVB = new CHSceneVertex[(*lpScene)->dwVecCount];
            fread((*lpScene)->lpVB,
                  sizeof(CHSceneVertex),
                  (*lpScene)->dwVecCount,
                  file);

            // Load triangle count
            fread(&(*lpScene)->dwTriCount, sizeof(DWORD), 1, file);
            // Load index data
            (*lpScene)->lpIB = new WORD[(*lpScene)->dwTriCount * 3];
            fread((*lpScene)->lpIB,
                  sizeof(WORD),
                  (*lpScene)->dwTriCount * 3,
                  file);

            // Load main texture
            fread(&temp, sizeof(DWORD), 1, file);
            (*lpScene)->lpTexName = new char[temp + 1];
            fread((*lpScene)->lpTexName, 1, temp, file);
            (*lpScene)->lpTexName[temp] = '\0';
            
            // Load texture
            CHTexture* tex;
            (*lpScene)->nTex = Texture_Load(&tex, (*lpScene)->lpTexName);
            if ((*lpScene)->nTex == -1)
            {
                fclose(file);
                Scene_Unload(lpScene);
                return FALSE;
            }

            // Load lightmap
            fread(&temp, sizeof(DWORD), 1, file);
            if (temp > 0)
            {
                (*lpScene)->lplTexName = new char[temp + 1];
                fread((*lpScene)->lplTexName, 1, temp, file);
                (*lpScene)->lplTexName[temp] = '\0';
                
                // Load lightmap texture
                CHTexture* ligtex;
                (*lpScene)->nlTex = Texture_Load(&ligtex, (*lpScene)->lplTexName);
                if ((*lpScene)->nlTex == -1)
                {
                    fclose(file);
                    Scene_Unload(lpScene);
                    return FALSE;
                }
            }

            // Load animation matrices
            fread(&(*lpScene)->dwFrameCount, sizeof(DWORD), 1, file);
            (*lpScene)->lpFrame = new XMMATRIX[(*lpScene)->dwFrameCount];
            fread((*lpScene)->lpFrame, sizeof(XMMATRIX), (*lpScene)->dwFrameCount, file);

            // Create DirectX 11 buffers
            if (FAILED(CHSceneInternal::CreateVertexBuffer(*lpScene)) ||
                FAILED(CHSceneInternal::CreateIndexBuffer(*lpScene)))
            {
                fclose(file);
                Scene_Unload(lpScene);
                return FALSE;
            }

            break;
        }
        else
        {
            fseek(file, chunk.dwChunkSize, SEEK_CUR);
        }
    }

    fclose(file);
    return TRUE;
}

CH_CORE_DLL_API
BOOL Scene_Save(char* lpName, CHScene* lpScene, BOOL bNew)
{
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);

    // Scene chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'S';
    chunk.byChunkID[1] = 'C';
    chunk.byChunkID[2] = 'E';
    chunk.byChunkID[3] = 'N';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);

    // Name
    DWORD length = static_cast<DWORD>(strlen(lpScene->lpName));
    fwrite(&length, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);

    fwrite(lpScene->lpName, sizeof(char), length, file);
    chunk.dwChunkSize += sizeof(char) * length;

    // Vertex count
    fwrite(&lpScene->dwVecCount, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    // Vertices
    fwrite(lpScene->lpVB, sizeof(CHSceneVertex), lpScene->dwVecCount, file);
    chunk.dwChunkSize += sizeof(CHSceneVertex) * lpScene->dwVecCount;

    // Triangle count
    fwrite(&lpScene->dwTriCount, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    // Triangles
    fwrite(lpScene->lpIB, sizeof(WORD), lpScene->dwTriCount * 3, file);
    chunk.dwChunkSize += sizeof(WORD) * lpScene->dwTriCount * 3;

    // Texture
    length = static_cast<DWORD>(strlen(lpScene->lpTexName));
    fwrite(&length, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);

    fwrite(lpScene->lpTexName, sizeof(char), length, file);
    chunk.dwChunkSize += sizeof(char) * length;

    // Lightmap
    if (lpScene->lplTexName)
        length = static_cast<DWORD>(strlen(lpScene->lplTexName));
    else
        length = 0;
    fwrite(&length, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);

    if (length > 0)
    {
        fwrite(lpScene->lplTexName, sizeof(char), length, file);
        chunk.dwChunkSize += sizeof(char) * length;
    }

    // Matrices
    fwrite(&lpScene->dwFrameCount, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    fwrite(lpScene->lpFrame, sizeof(XMMATRIX), lpScene->dwFrameCount, file);
    chunk.dwChunkSize += sizeof(XMMATRIX) * lpScene->dwFrameCount;

    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);

    fclose(file);
    return TRUE;
}

CH_CORE_DLL_API
void Scene_Unload(CHScene** lpScene)
{
    if (!lpScene || !*lpScene)
        return;

    delete[] (*lpScene)->lpName;
    delete[] (*lpScene)->lpVB;
    delete[] (*lpScene)->lpIB;

    delete[] (*lpScene)->lpTexName;
    if ((*lpScene)->nTex > -1)
        Texture_Unload(&g_lpTex[(*lpScene)->nTex]);

    delete[] (*lpScene)->lplTexName;
    if ((*lpScene)->nlTex > -1)
        Texture_Unload(&g_lpTex[(*lpScene)->nlTex]);

    delete[] (*lpScene)->lpFrame;

    // Release DirectX 11 buffers
    CHSceneInternal::ReleaseBuffers(*lpScene);

    delete *lpScene;
    *lpScene = nullptr;
}

// Optimization function (maintaining exact same algorithm)
CH_CORE_DLL_API
BOOL Scene_Optimize(CHScene* lpScene)
{
    CHSceneVertex* vertex = new CHSceneVertex[lpScene->dwVecCount];
    DWORD veccount = 0;
    WORD* tri = new WORD[lpScene->dwTriCount * 3];

    // Remove duplicate vertices
    int index;
    for (DWORD f = 0; f < lpScene->dwTriCount; f++)
    {
        for (DWORD i = 0; i < 3; i++)
        {
            index = lpScene->lpIB[f * 3 + i];

            for (DWORD c = 0; c < veccount; c++)
            {
                if (memcmp(&lpScene->lpVB[index],
                          &vertex[c],
                          sizeof(CHSceneVertex)) == 0)
                    break;
            }
            if (c == veccount)
            {
                // Not found same vertex
                CopyMemory(&vertex[veccount],
                          &lpScene->lpVB[index],
                          sizeof(CHSceneVertex));
                tri[f * 3 + i] = static_cast<WORD>(veccount);
                veccount++;
            }
            else
            {
                // Found same vertex
                tri[f * 3 + i] = static_cast<WORD>(c);
            }
        }
    }

    delete[] lpScene->lpVB;

    lpScene->dwVecCount = veccount;
    lpScene->lpVB = new CHSceneVertex[lpScene->dwVecCount];
    CopyMemory(lpScene->lpVB, vertex, sizeof(CHSceneVertex) * veccount);

    CopyMemory(lpScene->lpIB, tri, sizeof(WORD) * lpScene->dwTriCount * 3);

    delete[] vertex;
    delete[] tri;

    // Recreate DirectX 11 buffers with optimized data
    CHSceneInternal::ReleaseBuffers(lpScene);
    if (FAILED(CHSceneInternal::CreateVertexBuffer(lpScene)) ||
        FAILED(CHSceneInternal::CreateIndexBuffer(lpScene)))
    {
        return FALSE;
    }

    return TRUE;
}

CH_CORE_DLL_API
void Scene_Prepare()
{
    CHSceneInternal::SetupSceneRenderStates();
}

CH_CORE_DLL_API
BOOL Scene_Draw(CHScene* lpScene)
{
    if (!lpScene || lpScene->nTex < 0 || !g_lpTex[lpScene->nTex])
        return FALSE;

    // Set alpha blending based on texture format
    if (CHSceneInternal::ShouldUseAlphaBlending(g_lpTex[lpScene->nTex]))
    {
        SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
        SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCALPHA);
        SetRenderState(CH_RS_DESTBLEND, CH_BLEND_INVSRCALPHA);
    }
    else
    {
        SetRenderState(CH_RS_ALPHABLENDENABLE, FALSE);
    }

    // Set main texture
    if (!SetTexture(0, g_lpTex[lpScene->nTex]->lpSRV.Get()))
        return FALSE;

    // Set lightmap if available
    if (lpScene->nlTex > -1 && g_lpTex[lpScene->nlTex])
    {
        if (!SetTexture(1, g_lpTex[lpScene->nlTex]->lpSRV.Get()))
            return FALSE;
        CHSceneInternal::SetupLightmapRenderStates();
    }
    else
    {
        SetTexture(1, nullptr);
        CHSceneInternal::DisableLightmapRenderStates();
    }

    // Set world matrix (frame animation + additional transformation)
    XMMATRIX worldMatrix = XMMatrixIdentity();
    if (lpScene->dwFrameCount > 0 && lpScene->lpFrame)
    {
        worldMatrix = lpScene->lpFrame[lpScene->nFrame % lpScene->dwFrameCount];
    }
    worldMatrix = XMMatrixMultiply(worldMatrix, lpScene->matrix);

    // Update constant buffer for shaders
    CHInternal::g_ShaderManager.UpdateConstantBuffer(worldMatrix, g_ViewMatrix, g_ProjectMatrix);

    // Render the scene
    HRESULT hr = CHSceneInternal::RenderScene(lpScene);

    // Reset transformation matrix
    lpScene->matrix = XMMatrixIdentity();

    return SUCCEEDED(hr);
}

CH_CORE_DLL_API
void Scene_NextFrame(CHScene* lpScene, int nStep)
{
    if (lpScene->dwFrameCount > 0)
    {
        lpScene->nFrame = (lpScene->nFrame + nStep) % lpScene->dwFrameCount;
    }
}

CH_CORE_DLL_API
void Scene_Muliply(CHScene* lpScene, XMMATRIX* matrix)
{
    lpScene->matrix = XMMatrixMultiply(lpScene->matrix, *matrix);
}

// Internal implementation
namespace CHSceneInternal {

void SetupSceneRenderStates()
{
    SetRenderState(CH_RS_ZENABLE, TRUE);
    SetRenderState(CH_RS_ZWRITEENABLE, TRUE);
    SetRenderState(CH_RS_CULLMODE, CH_CULL_CW);

    SetTextureStageState(0, CH_TSS_COLORARG1, 1); // D3DTA_TEXTURE equivalent
    SetTextureStageState(0, CH_TSS_COLORARG2, 0); // D3DTA_DIFFUSE equivalent
    SetTextureStageState(0, CH_TSS_COLOROP, 4);   // D3DTOP_MODULATE equivalent

    SetTextureStageState(0, CH_TSS_ALPHAARG1, 0); // D3DTA_DIFFUSE
    SetTextureStageState(0, CH_TSS_ALPHAARG2, 0); // D3DTA_DIFFUSE
    SetTextureStageState(0, CH_TSS_ALPHAOP, 2);   // D3DTOP_SELECTARG2

    SetTextureStageState(0, CH_TSS_MINFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(0, CH_TSS_MAGFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(0, CH_TSS_MIPFILTER, CH_TEXF_LINEAR);
}

void SetupLightmapRenderStates()
{
    SetTextureStageState(1, CH_TSS_COLORARG1, 1); // D3DTA_TEXTURE
    SetTextureStageState(1, CH_TSS_COLORARG2, 2); // D3DTA_CURRENT
    SetTextureStageState(1, CH_TSS_COLOROP, 8);   // D3DTOP_MODULATE2X equivalent

    SetTextureStageState(1, CH_TSS_ALPHAARG1, 0); // D3DTA_DIFFUSE
    SetTextureStageState(1, CH_TSS_ALPHAARG2, 2); // D3DTA_CURRENT
    SetTextureStageState(1, CH_TSS_ALPHAOP, 1);   // D3DTOP_DISABLE

    SetTextureStageState(1, CH_TSS_MINFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(1, CH_TSS_MAGFILTER, CH_TEXF_LINEAR);
    SetTextureStageState(1, CH_TSS_MIPFILTER, CH_TEXF_NONE);
}

void DisableLightmapRenderStates()
{
    SetTextureStageState(1, CH_TSS_COLOROP, 1); // D3DTOP_DISABLE
    SetTextureStageState(1, CH_TSS_ALPHAOP, 1); // D3DTOP_DISABLE
}

HRESULT CreateVertexBuffer(CHScene* scene)
{
    if (!scene || !scene->lpVB || scene->dwVecCount == 0)
        return E_INVALIDARG;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(CHSceneVertex) * scene->dwVecCount;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = scene->lpVB;

    return g_D3DDevice->CreateBuffer(&bufferDesc, &initData, scene->vertexBuffer.GetAddressOf());
}

HRESULT CreateIndexBuffer(CHScene* scene)
{
    if (!scene || !scene->lpIB || scene->dwTriCount == 0)
        return E_INVALIDARG;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(WORD) * scene->dwTriCount * 3;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = scene->lpIB;

    return g_D3DDevice->CreateBuffer(&bufferDesc, &initData, scene->indexBuffer.GetAddressOf());
}

void ReleaseBuffers(CHScene* scene)
{
    if (scene)
    {
        scene->vertexBuffer.Reset();
        scene->indexBuffer.Reset();
    }
}

HRESULT RenderScene(CHScene* scene)
{
    if (!scene || !scene->vertexBuffer || !scene->indexBuffer)
        return E_INVALIDARG;

    // Set vertex buffer
    ID3D11Buffer* vb = scene->vertexBuffer.Get();
    g_D3DContext->IASetVertexBuffers(0, 1, &vb, &scene->vertexStride, &scene->vertexOffset);

    // Set index buffer
    g_D3DContext->IASetIndexBuffer(scene->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set default shaders
    CHInternal::g_ShaderManager.SetDefaultShaders();

    // Draw
    g_D3DContext->DrawIndexed(scene->dwTriCount * 3, 0, 0);

    return S_OK;
}

bool ShouldUseAlphaBlending(CHTexture* texture)
{
    if (!texture)
        return false;

    // Check if texture format has alpha channel
    CHFormat format = texture->Info.Format;
    return (format == CH_FMT_A8R8G8B8 ||
            format == CH_FMT_A1R5G5B5 ||
            format == CH_FMT_A4R4G4B4);
}

} // namespace CHSceneInternal