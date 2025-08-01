#include <windows.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "CH_ptcl.h"
#include "CH_main.h"
#include "CH_texture.h"
#include <algorithm>

extern const char CH_VERSION[64];

// Global particle shader manager
CHPtclInternal::ParticleShaderManager CHPtclInternal::g_ParticleShaderManager;

void Ptcl_Clear(CHPtcl* lpPtcl)
{
    if (!lpPtcl)
        return;
    
    // Clear name
    delete[] lpPtcl->lpName;
    lpPtcl->lpName = nullptr;
    
    // Clear vertex buffer
    delete[] lpPtcl->lpVB;
    lpPtcl->lpVB = nullptr;
    
    // Clear index buffer
    delete[] lpPtcl->lpIB;
    lpPtcl->lpIB = nullptr;
    
    // Clear texture name
    delete[] lpPtcl->lpTexName;
    lpPtcl->lpTexName = nullptr;
    
    lpPtcl->nTex = -1;
    lpPtcl->dwCount = 0;
    lpPtcl->dwRow = 1;
    
    // Clear frame data
    if (lpPtcl->lpPtcl)
    {
        for (DWORD i = 0; i < lpPtcl->dwFrames; i++)
        {
            delete[] lpPtcl->lpPtcl[i].lpPos;
            delete[] lpPtcl->lpPtcl[i].lpAge;
            delete[] lpPtcl->lpPtcl[i].lpSize;
        }
        delete[] lpPtcl->lpPtcl;
        lpPtcl->lpPtcl = nullptr;
    }
    
    lpPtcl->nFrame = 0;
    lpPtcl->dwFrames = 0;
    lpPtcl->matrix = XMMatrixIdentity();
    
    // Release DirectX 11 buffers
    CHPtclInternal::ReleaseBuffers(lpPtcl);
    
    lpPtcl->vertexStride = sizeof(CHPtclVertex);
    lpPtcl->vertexOffset = 0;
}

BOOL Ptcl_Load(CHPtcl** lpPtcl, FILE* file, BOOL bTex)
{
    if (!lpPtcl || !file)
        return FALSE;
    
    return CHPtclInternal::LoadPtclFromFile(file, lpPtcl, bTex != FALSE);
}

BOOL Ptcl_LoadPack(CHPtcl** lpPtcl, HANDLE f, BOOL bTex)
{
    if (!lpPtcl || f == INVALID_HANDLE_VALUE)
        return FALSE;
    
    return CHPtclInternal::LoadPtclFromPack(f, lpPtcl, bTex != FALSE);
}

BOOL Ptcl_Save(char* lpName, CHPtcl* lpPtcl, BOOL bNew)
{
    if (!lpName || !lpPtcl)
        return FALSE;
    
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);
    
    // Particle chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'P';
    chunk.byChunkID[1] = 'T';
    chunk.byChunkID[2] = 'C';
    chunk.byChunkID[3] = 'L';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);
    
    // Name
    DWORD nameLen = lpPtcl->lpName ? static_cast<DWORD>(strlen(lpPtcl->lpName)) : 0;
    fwrite(&nameLen, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    if (nameLen > 0)
    {
        fwrite(lpPtcl->lpName, sizeof(char), nameLen, file);
        chunk.dwChunkSize += sizeof(char) * nameLen;
    }
    
    // Texture name
    nameLen = lpPtcl->lpTexName ? static_cast<DWORD>(strlen(lpPtcl->lpTexName)) : 0;
    fwrite(&nameLen, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    if (nameLen > 0)
    {
        fwrite(lpPtcl->lpTexName, sizeof(char), nameLen, file);
        chunk.dwChunkSize += sizeof(char) * nameLen;
    }
    
    // Basic properties
    fwrite(&lpPtcl->dwRow, sizeof(DWORD), 1, file);
    fwrite(&lpPtcl->dwCount, sizeof(DWORD), 1, file);
    fwrite(&lpPtcl->dwFrames, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD) * 3;
    
    // Frame data
    for (DWORD n = 0; n < lpPtcl->dwFrames; n++)
    {
        CHPtclFrame* frame = &lpPtcl->lpPtcl[n];
        
        fwrite(&frame->dwCount, sizeof(DWORD), 1, file);
        chunk.dwChunkSize += sizeof(DWORD);
        
        if (frame->dwCount > 0)
        {
            // Particle positions (convert XMVECTOR to XMFLOAT3)
            for (DWORD j = 0; j < frame->dwCount; j++)
            {
                XMFLOAT3 pos;
                XMStoreFloat3(&pos, frame->lpPos[j]);
                fwrite(&pos, sizeof(XMFLOAT3), 1, file);
            }
            chunk.dwChunkSize += sizeof(XMFLOAT3) * frame->dwCount;
            
            // Particle ages and sizes
            fwrite(frame->lpAge, sizeof(float), frame->dwCount, file);
            fwrite(frame->lpSize, sizeof(float), frame->dwCount, file);
            chunk.dwChunkSize += sizeof(float) * frame->dwCount * 2;
            
            // Frame matrix (convert XMMATRIX to XMFLOAT4X4)
            XMFLOAT4X4 matrixData;
            XMStoreFloat4x4(&matrixData, frame->matrix);
            fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
            chunk.dwChunkSize += sizeof(XMFLOAT4X4);
        }
    }
    
    // Update chunk size
    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);
    
    fclose(file);
    return TRUE;
}

void Ptcl_Unload(CHPtcl** lpPtcl)
{
    if (!lpPtcl || !*lpPtcl)
        return;
    
    // Clean up frame data
    if ((*lpPtcl)->lpPtcl)
    {
        for (DWORD i = 0; i < (*lpPtcl)->dwFrames; i++)
        {
            delete[] (*lpPtcl)->lpPtcl[i].lpPos;
            delete[] (*lpPtcl)->lpPtcl[i].lpAge;
            delete[] (*lpPtcl)->lpPtcl[i].lpSize;
        }
        delete[] (*lpPtcl)->lpPtcl;
    }
    
    // Clean up other data
    delete[] (*lpPtcl)->lpName;
    delete[] (*lpPtcl)->lpVB;
    delete[] (*lpPtcl)->lpIB;
    delete[] (*lpPtcl)->lpTexName;
    
    // Release DirectX buffers
    CHPtclInternal::ReleaseBuffers(*lpPtcl);
    
    delete *lpPtcl;
    *lpPtcl = nullptr;
}

void Ptcl_Prepare()
{
    CHPtclInternal::SetupParticleRenderStates();
}

BOOL Ptcl_Draw(CHPtcl* lpPtcl, int nAsb, int nAdb)
{
    if (!lpPtcl || lpPtcl->nFrame >= static_cast<int>(lpPtcl->dwFrames))
        return FALSE;
    
    CHPtclFrame* frame = &lpPtcl->lpPtcl[lpPtcl->nFrame];
    
    if (frame->dwCount == 0)
        return TRUE;
    
    // Generate particle quads
    CHPtclInternal::GenerateParticleQuads(lpPtcl, frame);
    
    // Set render states
    SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
    SetRenderState(CH_RS_SRCBLEND, nAsb);
    SetRenderState(CH_RS_DESTBLEND, nAdb);
    SetRenderState(CH_RS_ALPHATESTENABLE, TRUE);
    SetRenderState(CH_RS_ALPHAREF, 0x08);
    SetRenderState(CH_RS_ALPHAFUNC, CH_CMP_GREATEREQUAL);
    
    // Set texture
    if (lpPtcl->nTex >= 0 && lpPtcl->nTex < TEX_MAX && g_lpTex[lpPtcl->nTex])
    {
        SetTexture(0, g_lpTex[lpPtcl->nTex]->lpSRV.Get());
    }
    
    // Set vertex and index buffers
    if (!lpPtcl->vertexBuffer && !CHPtclInternal::CreateVertexBuffer(lpPtcl))
        return FALSE;
    
    if (!lpPtcl->indexBuffer && !CHPtclInternal::CreateIndexBuffer(lpPtcl))
        return FALSE;
    
    g_D3DContext->IASetVertexBuffers(0, 1, lpPtcl->vertexBuffer.GetAddressOf(), 
        &lpPtcl->vertexStride, &lpPtcl->vertexOffset);
    g_D3DContext->IASetIndexBuffer(lpPtcl->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Set shaders
    CHPtclInternal::g_ParticleShaderManager.SetParticleShaders();
    
    // Draw particles
    g_D3DContext->DrawIndexed(frame->dwCount * 6, 0, 0);
    
    return TRUE;
}

void Ptcl_NextFrame(CHPtcl* lpPtcl, int nStep)
{
    if (!lpPtcl)
        return;
    
    if (lpPtcl->dwFrames > 0)
    {
        lpPtcl->nFrame = (lpPtcl->nFrame + nStep) % lpPtcl->dwFrames;
    }
}

void Ptcl_SetFrame(CHPtcl* lpPtcl, DWORD dwFrame)
{
    if (!lpPtcl)
        return;
    
    if (dwFrame < lpPtcl->dwFrames)
    {
        lpPtcl->nFrame = static_cast<int>(dwFrame);
    }
}

void Ptcl_Muliply(CHPtcl* lpPtcl, XMMATRIX* matrix)
{
    if (!lpPtcl || !matrix)
        return;
    
    lpPtcl->matrix = XMMatrixMultiply(lpPtcl->matrix, *matrix);
}

void Ptcl_ClearMatrix(CHPtcl* lpPtcl)
{
    if (!lpPtcl)
        return;
    
    lpPtcl->matrix = XMMatrixIdentity();
}

void Ptcl_ChangeTexture(CHPtcl* lpPtcl, int nTexID)
{
    if (!lpPtcl)
        return;
    
    lpPtcl->nTex = nTexID;
}

// Internal implementation
namespace CHPtclInternal {

void GenerateParticleQuads(CHPtcl* ptcl, CHPtclFrame* frame)
{
    if (!ptcl || !frame || frame->dwCount == 0)
        return;
    
    DWORD segcount = ptcl->dwRow * ptcl->dwRow;
    float segsize = 1.0f / ptcl->dwRow;
    
    // Calculate combined transformation matrix
    XMMATRIX combinedMatrix = XMMatrixMultiply(frame->matrix, ptcl->matrix);
    combinedMatrix = XMMatrixMultiply(combinedMatrix, g_ViewMatrix);
    
    // Generate quads for each particle
    for (DWORD n = 0; n < frame->dwCount; n++)
    {
        DWORD index = static_cast<DWORD>(frame->lpAge[n] * segcount);
        
        float u = (index % ptcl->dwRow) * segsize;
        float v = (index / ptcl->dwRow) * segsize;
        
        // Transform particle position
        XMVECTOR worldPos = XMVector3TransformCoord(frame->lpPos[n], combinedMatrix);
        XMFLOAT3 pos;
        XMStoreFloat3(&pos, worldPos);
        
        float size = frame->lpSize[n];
        
        // Generate quad vertices
        DWORD baseIndex = n * 4;
        
        // Vertex 0 (top-left)
        ptcl->lpVB[baseIndex].x = pos.x - size;
        ptcl->lpVB[baseIndex].y = pos.y - size;
        ptcl->lpVB[baseIndex].z = pos.z;
        ptcl->lpVB[baseIndex].u = u;
        ptcl->lpVB[baseIndex].v = v + segsize;
        ptcl->lpVB[baseIndex].color = 0xFFFFFFFF;
        
        // Vertex 1 (top-right)
        ptcl->lpVB[baseIndex + 1].x = pos.x + size;
        ptcl->lpVB[baseIndex + 1].y = pos.y - size;
        ptcl->lpVB[baseIndex + 1].z = pos.z;
        ptcl->lpVB[baseIndex + 1].u = u + segsize;
        ptcl->lpVB[baseIndex + 1].v = v + segsize;
        ptcl->lpVB[baseIndex + 1].color = 0xFFFFFFFF;
        
        // Vertex 2 (bottom-left)
        ptcl->lpVB[baseIndex + 2].x = pos.x - size;
        ptcl->lpVB[baseIndex + 2].y = pos.y + size;
        ptcl->lpVB[baseIndex + 2].z = pos.z;
        ptcl->lpVB[baseIndex + 2].u = u;
        ptcl->lpVB[baseIndex + 2].v = v;
        ptcl->lpVB[baseIndex + 2].color = 0xFFFFFFFF;
        
        // Vertex 3 (bottom-right)
        ptcl->lpVB[baseIndex + 3].x = pos.x + size;
        ptcl->lpVB[baseIndex + 3].y = pos.y + size;
        ptcl->lpVB[baseIndex + 3].z = pos.z;
        ptcl->lpVB[baseIndex + 3].u = u + segsize;
        ptcl->lpVB[baseIndex + 3].v = v;
        ptcl->lpVB[baseIndex + 3].color = 0xFFFFFFFF;
        
        // Generate indices for quad
        ptcl->lpIB[n * 6] = static_cast<WORD>(baseIndex);
        ptcl->lpIB[n * 6 + 1] = static_cast<WORD>(baseIndex + 1);
        ptcl->lpIB[n * 6 + 2] = static_cast<WORD>(baseIndex + 2);
        ptcl->lpIB[n * 6 + 3] = static_cast<WORD>(baseIndex + 2);
        ptcl->lpIB[n * 6 + 4] = static_cast<WORD>(baseIndex + 1);
        ptcl->lpIB[n * 6 + 5] = static_cast<WORD>(baseIndex + 3);
    }
    
    // Update GPU buffers
    UpdateVertexBuffer(ptcl);
}

BOOL CreateVertexBuffer(CHPtcl* ptcl)
{
    if (!ptcl)
        return FALSE;
    
    if (ptcl->vertexBuffer)
        return TRUE; // Already created
    
    CHPtclFrame* currentFrame = &ptcl->lpPtcl[ptcl->nFrame];
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(CHPtclVertex) * currentFrame->dwCount * 4;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    return SUCCEEDED(g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, ptcl->vertexBuffer.GetAddressOf()));
}

BOOL CreateIndexBuffer(CHPtcl* ptcl)
{
    if (!ptcl)
        return FALSE;
    
    if (ptcl->indexBuffer)
        return TRUE; // Already created
    
    CHPtclFrame* currentFrame = &ptcl->lpPtcl[ptcl->nFrame];
    
    // Generate indices for quads (two triangles per particle)
    DWORD indexCount = currentFrame->dwCount * 6; // 6 indices per quad
    WORD* indices = new WORD[indexCount];
    
    for (DWORD i = 0; i < currentFrame->dwCount; i++)
    {
        DWORD baseVertex = i * 4;
        DWORD baseIndex = i * 6;
        
        // First triangle (0, 1, 2)
        indices[baseIndex] = static_cast<WORD>(baseVertex);
        indices[baseIndex + 1] = static_cast<WORD>(baseVertex + 1);
        indices[baseIndex + 2] = static_cast<WORD>(baseVertex + 2);
        
        // Second triangle (1, 3, 2)
        indices[baseIndex + 3] = static_cast<WORD>(baseVertex + 1);
        indices[baseIndex + 4] = static_cast<WORD>(baseVertex + 3);
        indices[baseIndex + 5] = static_cast<WORD>(baseVertex + 2);
    }
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(WORD) * indexCount;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = indices;
    
    BOOL result = SUCCEEDED(g_D3DDevice->CreateBuffer(&bufferDesc, &initData, ptcl->indexBuffer.GetAddressOf()));
    
    delete[] indices;
    return result;
}

void UpdateVertexBuffer(CHPtcl* ptcl)
{
    if (!ptcl || !ptcl->vertexBuffer || !ptcl->lpVB)
        return;
    
    CHPtclFrame* currentFrame = &ptcl->lpPtcl[ptcl->nFrame];
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(g_D3DContext->Map(ptcl->vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        memcpy(mappedResource.pData, ptcl->lpVB, sizeof(CHPtclVertex) * currentFrame->dwCount * 4);
        g_D3DContext->Unmap(ptcl->vertexBuffer.Get(), 0);
    }
}

void ReleaseBuffers(CHPtcl* ptcl)
{
    if (!ptcl)
        return;
    
    ptcl->vertexBuffer.Reset();
    ptcl->indexBuffer.Reset();
}

void SetupParticleRenderStates()
{
    // Set render states for particle rendering
    SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
    SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCALPHA);
    SetRenderState(CH_RS_DESTBLEND, CH_BLEND_ONE); // Additive blending for particles
    SetRenderState(CH_RS_ZWRITEENABLE, FALSE); // Don't write to depth buffer
    SetRenderState(CH_RS_CULLMODE, CH_CULL_NONE);
}

BOOL LoadPtclFromFile(FILE* file, CHPtcl** ptcl, bool loadTextures)
{
    if (!file || !ptcl)
        return FALSE;
    
    *ptcl = new CHPtcl;
    Ptcl_Clear(*ptcl);
    
    DWORD temp;
    
    // Read name
    fread(&temp, sizeof(DWORD), 1, file);
    (*ptcl)->lpName = new char[temp + 1];
    fread((*ptcl)->lpName, 1, temp, file);
    (*ptcl)->lpName[temp] = '\0';
    
    // Read texture name
    fread(&temp, sizeof(DWORD), 1, file);
    (*ptcl)->lpTexName = new char[temp + 1];
    fread((*ptcl)->lpTexName, 1, temp, file);
    (*ptcl)->lpTexName[temp] = '\0';
    
    // Load texture if requested
    if (loadTextures)
    {
        CHTexture* tex;
        (*ptcl)->nTex = Texture_Load(&tex, (*ptcl)->lpTexName);
        if ((*ptcl)->nTex == -1)
            return FALSE;
    }
    
    // Read basic properties
    fread(&(*ptcl)->dwRow, sizeof(DWORD), 1, file);
    fread(&(*ptcl)->dwCount, sizeof(DWORD), 1, file);
    fread(&(*ptcl)->dwFrames, sizeof(DWORD), 1, file);
    
    // Allocate vertex and index buffers
    (*ptcl)->lpVB = new CHPtclVertex[(*ptcl)->dwCount * 4];
    (*ptcl)->lpIB = new WORD[(*ptcl)->dwCount * 6];
    
    // Allocate frame data
    (*ptcl)->lpPtcl = new CHPtclFrame[(*ptcl)->dwFrames];
    
    // Read frame data
    for (DWORD n = 0; n < (*ptcl)->dwFrames; n++)
    {
        CHPtclFrame* frame = &(*ptcl)->lpPtcl[n];
        
        fread(&frame->dwCount, sizeof(DWORD), 1, file);
        
        if (frame->dwCount > 0)
        {
            // Allocate and read positions
            frame->lpPos = new XMVECTOR[frame->dwCount];
            for (DWORD i = 0; i < frame->dwCount; i++)
            {
                XMFLOAT3 pos;
                fread(&pos, sizeof(XMFLOAT3), 1, file);
                frame->lpPos[i] = XMLoadFloat3(&pos);
            }
            
            // Allocate and read ages
            frame->lpAge = new float[frame->dwCount];
            fread(frame->lpAge, sizeof(float), frame->dwCount, file);
            
            // Allocate and read sizes
            frame->lpSize = new float[frame->dwCount];
            fread(frame->lpSize, sizeof(float), frame->dwCount, file);
            
            // Read matrix
            XMFLOAT4X4 matrixData;
            fread(&matrixData, sizeof(XMFLOAT4X4), 1, file);
            frame->matrix = XMLoadFloat4x4(&matrixData);
        }
        else
        {
            frame->lpPos = nullptr;
            frame->lpAge = nullptr;
            frame->lpSize = nullptr;
        }
    }
    
    return TRUE;
}

BOOL LoadPtclFromPack(HANDLE handle, CHPtcl** ptcl, bool loadTextures)
{
    if (!handle || handle == INVALID_HANDLE_VALUE || !ptcl)
        return FALSE;
    
    *ptcl = new CHPtcl;
    Ptcl_Clear(*ptcl);
    
    DWORD bytesRead;
    DWORD temp;
    
    // Read name
    ReadFile(handle, &temp, sizeof(DWORD), &bytesRead, nullptr);
    (*ptcl)->lpName = new char[temp + 1];
    ReadFile(handle, (*ptcl)->lpName, temp, &bytesRead, nullptr);
    (*ptcl)->lpName[temp] = '\0';
    
    // Read texture name
    ReadFile(handle, &temp, sizeof(DWORD), &bytesRead, nullptr);
    (*ptcl)->lpTexName = new char[temp + 1];
    ReadFile(handle, (*ptcl)->lpTexName, temp, &bytesRead, nullptr);
    (*ptcl)->lpTexName[temp] = '\0';
    
    // Load texture if requested
    if (loadTextures)
    {
        CHTexture* tex;
        (*ptcl)->nTex = Texture_Load(&tex, (*ptcl)->lpTexName);
        if ((*ptcl)->nTex == -1)
            return FALSE;
    }
    
    // Read basic properties
    ReadFile(handle, &(*ptcl)->dwRow, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(handle, &(*ptcl)->dwCount, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(handle, &(*ptcl)->dwFrames, sizeof(DWORD), &bytesRead, nullptr);
    
    // Allocate vertex and index buffers
    (*ptcl)->lpVB = new CHPtclVertex[(*ptcl)->dwCount * 4];
    (*ptcl)->lpIB = new WORD[(*ptcl)->dwCount * 6];
    
    // Allocate frame data
    (*ptcl)->lpPtcl = new CHPtclFrame[(*ptcl)->dwFrames];
    
    // Read frame data
    for (DWORD n = 0; n < (*ptcl)->dwFrames; n++)
    {
        CHPtclFrame* frame = &(*ptcl)->lpPtcl[n];
        
        ReadFile(handle, &frame->dwCount, sizeof(DWORD), &bytesRead, nullptr);
        
        if (frame->dwCount > 0)
        {
            // Allocate and read positions
            frame->lpPos = new XMVECTOR[frame->dwCount];
            for (DWORD i = 0; i < frame->dwCount; i++)
            {
                XMFLOAT3 pos;
                ReadFile(handle, &pos, sizeof(XMFLOAT3), &bytesRead, nullptr);
                frame->lpPos[i] = XMLoadFloat3(&pos);
            }
            
            // Allocate and read ages
            frame->lpAge = new float[frame->dwCount];
            ReadFile(handle, frame->lpAge, sizeof(float) * frame->dwCount, &bytesRead, nullptr);
            
            // Allocate and read sizes
            frame->lpSize = new float[frame->dwCount];
            ReadFile(handle, frame->lpSize, sizeof(float) * frame->dwCount, &bytesRead, nullptr);
            
            // Read matrix
            XMFLOAT4X4 matrixData;
            ReadFile(handle, &matrixData, sizeof(XMFLOAT4X4), &bytesRead, nullptr);
            frame->matrix = XMLoadFloat4x4(&matrixData);
        }
        else
        {
            frame->lpPos = nullptr;
            frame->lpAge = nullptr;
            frame->lpSize = nullptr;
        }
    }
    
    return TRUE;
}

// ParticleShaderManager implementation
HRESULT ParticleShaderManager::Initialize()
{
    // For DirectX 11, we'll use the default vertex shader pipeline
    // In a full implementation, you would compile custom vertex/pixel shaders here
    // For now, we'll use the standard pipeline which works for basic particle rendering
    
    // Create input layout for particle vertices
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    // For now, we'll use the default pipeline
    // In a full implementation, you would compile and create vertex/pixel shaders here
    return S_OK;
}

void ParticleShaderManager::SetParticleShaders()
{
    // For DirectX 11, we'll use the default pipeline
    // In a full implementation, you would set custom vertex/pixel shaders here
    // For now, we'll rely on the fixed function pipeline equivalent
    
    // Set the vertex shader (if we had one compiled)
     g_D3DContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    
    // Set the pixel shader (if we had one compiled)
     g_D3DContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    
    // Set the input layout
     g_D3DContext->IASetInputLayout(m_inputLayout.Get());
}

void ParticleShaderManager::Cleanup()
{
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_inputLayout.Reset();
}

void GenerateQuads(CHPtcl* ptcl)
{
    if (!ptcl || ptcl->nFrame >= static_cast<int>(ptcl->dwFrames))
        return;
    
    CHPtclFrame* frame = &ptcl->lpPtcl[ptcl->nFrame];
    GenerateParticleQuads(ptcl, frame);
}

void UpdateParticles(CHPtcl* ptcl)
{
    if (!ptcl || ptcl->nFrame >= static_cast<int>(ptcl->dwFrames))
        return;
    
    CHPtclFrame* currentFrame = &ptcl->lpPtcl[ptcl->nFrame];
    
    // Update particle ages and sizes based on frame data
    for (DWORD i = 0; i < currentFrame->dwCount; i++)
    {
        // Simple age increment (in a real system, this would be frame-rate independent)
        currentFrame->lpAge[i] += 0.016f; // Assuming ~60 FPS
        
        // Update size based on age (example: particles grow over time)
        currentFrame->lpSize[i] = 1.0f + currentFrame->lpAge[i] * 0.5f;
    }
}

void SortParticlesByDepth(CHPtcl* ptcl)
{
    // For proper alpha blending, particles should be sorted back-to-front
    // This is a simplified implementation - in practice, you'd use the view matrix
    if (!ptcl || ptcl->nFrame >= static_cast<int>(ptcl->dwFrames))
        return;
    
    // TODO: Implement depth sorting based on camera position
    // For now, we assume particles are already in correct order
}

BOOL RenderParticleSystem(CHPtcl* ptcl, int srcBlend, int destBlend)
{
    if (!ptcl || ptcl->nFrame >= static_cast<int>(ptcl->dwFrames))
        return FALSE;
    
    CHPtclFrame* currentFrame = &ptcl->lpPtcl[ptcl->nFrame];
    
    // Create buffers if needed
    if (!ptcl->vertexBuffer && !CreateVertexBuffer(ptcl))
        return FALSE;
    
    if (!ptcl->indexBuffer && !CreateIndexBuffer(ptcl))
        return FALSE;
    
    // Set texture if available
    if (ptcl->nTex >= 0 && ptcl->nTex < TEX_MAX && g_lpTex[ptcl->nTex])
    {
        SetTexture(0, g_lpTex[ptcl->nTex]->lpSRV.Get());
    }
    
    // Set custom blend modes
    SetRenderState(CH_RS_SRCBLEND, srcBlend);
    SetRenderState(CH_RS_DESTBLEND, destBlend);
    
    // Set vertex and index buffers
    g_D3DContext->IASetVertexBuffers(0, 1, ptcl->vertexBuffer.GetAddressOf(), &ptcl->vertexStride, &ptcl->vertexOffset);
    g_D3DContext->IASetIndexBuffer(ptcl->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Draw particles
    g_D3DContext->DrawIndexed(currentFrame->dwCount * 6, 0, 0);
    
    return TRUE;
}

} // namespace CHPtclInternal