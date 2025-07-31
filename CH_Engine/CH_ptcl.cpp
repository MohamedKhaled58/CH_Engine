#include "CH_ptcl.h"
#include "CH_main.h"

extern const char CH_VERSION[64];

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
    
    // Basic properties
    fwrite(&lpPtcl->nTex, sizeof(int), 1, file);
    fwrite(&lpPtcl->dwCount, sizeof(DWORD), 1, file);
    fwrite(&lpPtcl->dwRow, sizeof(DWORD), 1, file);
    fwrite(&lpPtcl->dwFrames, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(int) + sizeof(DWORD) * 3;
    
    // Frame data
    for (DWORD i = 0; i < lpPtcl->dwFrames; i++)
    {
        CHPtclFrame* frame = &lpPtcl->lpPtcl[i];
        
        fwrite(&frame->dwCount, sizeof(DWORD), 1, file);
        chunk.dwChunkSize += sizeof(DWORD);
        
        // Particle positions (convert XMVECTOR to float arrays)
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
        
        // Frame matrix (convert XMMATRIX to float array)
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, frame->matrix);
        fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
        chunk.dwChunkSize += sizeof(XMFLOAT4X4);
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
    
    Ptcl_Clear(*lpPtcl);
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
    
    // Update particle system
    CHPtclInternal::UpdateParticles(lpPtcl);
    
    // Generate quad geometry for particles
    CHPtclInternal::GenerateQuads(lpPtcl);
    
    // Sort particles by depth for proper alpha blending
    CHPtclInternal::SortParticlesByDepth(lpPtcl);
    
    // Render the particle system
    return CHPtclInternal::RenderParticleSystem(lpPtcl, nAsb, nAdb);
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

void GenerateQuads(CHPtcl* ptcl)
{
    if (!ptcl || ptcl->nFrame >= static_cast<int>(ptcl->dwFrames))
        return;
    
    CHPtclFrame* currentFrame = &ptcl->lpPtcl[ptcl->nFrame];
    
    // Ensure vertex buffer is large enough
    if (!ptcl->lpVB)
    {
        ptcl->lpVB = new CHPtclVertex[currentFrame->dwCount * 4]; // 4 vertices per particle
    }
    
    // Generate billboard quads for each particle
    for (DWORD i = 0; i < currentFrame->dwCount; i++)
    {
        XMVECTOR worldPos = XMVector3TransformCoord(currentFrame->lpPos[i], ptcl->matrix);
        float size = currentFrame->lpSize[i];
        
        // Calculate billboard vectors (facing camera)
        XMVECTOR right = XMVectorSet(size, 0.0f, 0.0f, 0.0f);
        XMVECTOR up = XMVectorSet(0.0f, size, 0.0f, 0.0f);
        
        // Create quad vertices
        DWORD baseIndex = i * 4;
        
        // Top-left
        XMVECTOR pos = XMVectorSubtract(XMVectorSubtract(worldPos, right), up);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&ptcl->lpVB[baseIndex]), pos);
        ptcl->lpVB[baseIndex].color = 0xFFFFFFFF;
        ptcl->lpVB[baseIndex].u = 0.0f;
        ptcl->lpVB[baseIndex].v = 0.0f;
        
        // Top-right
        pos = XMVectorAdd(XMVectorSubtract(worldPos, up), right);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&ptcl->lpVB[baseIndex + 1]), pos);
        ptcl->lpVB[baseIndex + 1].color = 0xFFFFFFFF;
        ptcl->lpVB[baseIndex + 1].u = 1.0f;
        ptcl->lpVB[baseIndex + 1].v = 0.0f;
        
        // Bottom-left
        pos = XMVectorSubtract(XMVectorAdd(worldPos, up), right);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&ptcl->lpVB[baseIndex + 2]), pos);
        ptcl->lpVB[baseIndex + 2].color = 0xFFFFFFFF;
        ptcl->lpVB[baseIndex + 2].u = 0.0f;
        ptcl->lpVB[baseIndex + 2].v = 1.0f;
        
        // Bottom-right
        pos = XMVectorAdd(XMVectorAdd(worldPos, right), up);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&ptcl->lpVB[baseIndex + 3]), pos);
        ptcl->lpVB[baseIndex + 3].color = 0xFFFFFFFF;
        ptcl->lpVB[baseIndex + 3].u = 1.0f;
        ptcl->lpVB[baseIndex + 3].v = 1.0f;
    }
    
    // Update GPU vertex buffer
    UpdateVertexBuffer(ptcl);
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

BOOL LoadPtclFromFile(FILE* file, CHPtcl** ptcl, bool loadTextures)
{
    // Implementation similar to other Load functions
    // This is a placeholder for the full file loading implementation
    return FALSE;
}

BOOL LoadPtclFromPack(HANDLE handle, CHPtcl** ptcl, bool loadTextures)
{
    // Implementation for loading from packed files
    // This is a placeholder for the full packed file loading implementation
    return FALSE;
}

} // namespace CHPtclInternal