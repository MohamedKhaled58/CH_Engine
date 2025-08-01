#include "CH_shape.h"
#include "CH_main.h"

extern const char CH_VERSION[64];

void SMotion_Clear(CHSMotion* lpSMotion)
{
    if (!lpSMotion)
        return;
        
    delete[] lpSMotion->lpFrames;
    lpSMotion->lpFrames = nullptr;
    lpSMotion->dwFrames = 0;
    lpSMotion->matrix = XMMatrixIdentity();
    lpSMotion->nFrame = 0;
}

BOOL SMotion_Load(CHSMotion** lpSMotion, FILE* file)
{
    if (!lpSMotion || !file)
        return FALSE;
    
    return CHShapeInternal::LoadSMotionFromFile(file, lpSMotion);
}

BOOL SMotion_LoadPack(CHSMotion** lpSMotion, HANDLE f)
{
    if (!lpSMotion || f == INVALID_HANDLE_VALUE)
        return FALSE;
    
    return CHShapeInternal::LoadSMotionFromPack(f, lpSMotion);
}

BOOL SMotion_Save(char* lpName, CHSMotion* lpSMotion, BOOL bNew)
{
    if (!lpName || !lpSMotion)
        return FALSE;
    
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);
    
    // Shape motion chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'S';
    chunk.byChunkID[1] = 'M';
    chunk.byChunkID[2] = 'O';
    chunk.byChunkID[3] = 'T';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);
    
    // Frame count
    fwrite(&lpSMotion->dwFrames, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    
    // Frame matrices (convert XMMATRIX to float arrays)
    for (DWORD i = 0; i < lpSMotion->dwFrames; i++)
    {
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, lpSMotion->lpFrames[i]);
        fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
        chunk.dwChunkSize += sizeof(XMFLOAT4X4);
    }
    
    // Current matrix
    XMFLOAT4X4 currentMatrix;
    XMStoreFloat4x4(&currentMatrix, lpSMotion->matrix);
    fwrite(&currentMatrix, sizeof(XMFLOAT4X4), 1, file);
    chunk.dwChunkSize += sizeof(XMFLOAT4X4);
    
    // Current frame
    fwrite(&lpSMotion->nFrame, sizeof(int), 1, file);
    chunk.dwChunkSize += sizeof(int);
    
    // Update chunk size
    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);
    
    fclose(file);
    return TRUE;
}

void SMotion_Unload(CHSMotion** lpSMotion)
{
    if (!lpSMotion || !*lpSMotion)
        return;
    
    SMotion_Clear(*lpSMotion);
    delete *lpSMotion;
    *lpSMotion = nullptr;
}

void Shape_Clear(CHShape* lpShape)
{
    if (!lpShape)
        return;
    
    // Clear name
    delete[] lpShape->lpName;
    lpShape->lpName = nullptr;
    
    // Clear lines
    if (lpShape->lpLine)
    {
        for (DWORD i = 0; i < lpShape->dwLineCount; i++)
        {
            delete[] lpShape->lpLine[i].lpVB;
        }
        delete[] lpShape->lpLine;
        lpShape->lpLine = nullptr;
    }
    lpShape->dwLineCount = 0;
    
    // Clear texture name
    delete[] lpShape->lpTexName;
    lpShape->lpTexName = nullptr;
    lpShape->nTex = -1;
    
    // Clear motion
    if (lpShape->lpMotion)
    {
        SMotion_Unload(&lpShape->lpMotion);
    }
    
    // Clear flash data
    delete[] lpShape->vb;
    lpShape->vb = nullptr;
    lpShape->dwSegment = 0;
    lpShape->dwSegmentCur = 0;
    lpShape->last[0] = XMVectorZero();
    lpShape->last[1] = XMVectorZero();
    lpShape->bFirst = TRUE;
    
    // Clear smoothing data
    lpShape->dwSmooth = 1;
    delete[] lpShape->lpSmooths0;
    delete[] lpShape->lpSmooths1;
    lpShape->lpSmooths0 = nullptr;
    lpShape->lpSmooths1 = nullptr;
    
    // Clear TearAir data
    lpShape->pTearAirTex.Reset();
    delete[] lpShape->pScreenPnt;
    lpShape->pScreenPnt = nullptr;
    ZeroMemory(&lpShape->TearAirTexRect, sizeof(RECT));
    ZeroMemory(&lpShape->LastTearAirTexRect, sizeof(RECT));
    
    // Release DirectX 11 buffers
    CHShapeInternal::ReleaseBuffers(lpShape);
    
    lpShape->vertexStride = sizeof(CHShapeOutVertex);
    lpShape->vertexOffset = 0;
}

BOOL Shape_Load(CHShape** lpShape, FILE* file, BOOL bTex)
{
    if (!lpShape || !file)
        return FALSE;
    
    return CHShapeInternal::LoadShapeFromFile(file, lpShape, bTex != FALSE);
}

BOOL Shape_LoadPack(CHShape** lpShape, HANDLE f, BOOL bTex)
{
    if (!lpShape || f == INVALID_HANDLE_VALUE)
        return FALSE;
    
    return CHShapeInternal::LoadShapeFromPack(f, lpShape, bTex != FALSE);
}

BOOL Shape_Save(char* lpName, CHShape* lpShape, BOOL bNew)
{
    if (!lpName || !lpShape)
        return FALSE;
    
    return CHShapeInternal::SaveShapeToFile(lpName, lpShape, bNew != FALSE);
}

void Shape_Unload(CHShape** lpShape)
{
    if (!lpShape || !*lpShape)
        return;
    
    Shape_Clear(*lpShape);
    delete *lpShape;
    *lpShape = nullptr;
}

void Shape_SetSegment(CHShape* lpShape, DWORD dwSegment, DWORD dwSmooth)
{
    if (!lpShape)
        return;
    
    lpShape->dwSegment = dwSegment;
    lpShape->dwSmooth = dwSmooth;
    
    // Allocate vertex buffer for segments
    delete[] lpShape->vb;
    lpShape->vb = new CHShapeOutVertex[dwSegment * 2]; // Two vertices per segment
    
    // Allocate smoothing arrays if needed
    if (dwSmooth > 1)
    {
        delete[] lpShape->lpSmooths0;
        delete[] lpShape->lpSmooths1;
        lpShape->lpSmooths0 = new XMVECTOR[dwSegment];
        lpShape->lpSmooths1 = new XMVECTOR[dwSegment];
    }
}

BOOL Shape_Draw(CHShape* lpShape, BOOL bLocal, int nAsb, int nAdb)
{
    if (!lpShape)
        return FALSE;
    
    // Process shape lines and generate geometry
    CHShapeInternal::ProcessShapeLines(lpShape);
    CHShapeInternal::GenerateShapeGeometry(lpShape);
    
    // Render the shape
    return CHShapeInternal::RenderShape(lpShape, true, nAsb, nAdb);
}

void Shape_Prepare()
{
    CHShapeInternal::SetupShapeRenderStates();
}

void Shape_ChangeTexture(CHShape* lpShape, int nTexID)
{
    if (!lpShape)
        return;
    
    lpShape->nTex = nTexID;
}

void Shape_Muliply(CHShape* lpShape, XMMATRIX* matrix)
{
    if (!lpShape || !matrix)
        return;
    
    // Apply transformation to all lines
    for (DWORD i = 0; i < lpShape->dwLineCount; i++)
    {
        CHLine* line = &lpShape->lpLine[i];
        for (DWORD j = 0; j < line->dwVecCount; j++)
        {
            line->lpVB[j] = XMVector3TransformCoord(line->lpVB[j], *matrix);
        }
    }
    
    // Apply to motion matrix if available
    if (lpShape->lpMotion)
    {
        lpShape->lpMotion->matrix = XMMatrixMultiply(lpShape->lpMotion->matrix, *matrix);
    }
}

void Shape_NextFrame(CHShape* lpShape, int nStep)
{
    if (!lpShape || !lpShape->lpMotion)
        return;
    
    if (lpShape->lpMotion->dwFrames > 0)
    {
        lpShape->lpMotion->nFrame = (lpShape->lpMotion->nFrame + nStep) % lpShape->lpMotion->dwFrames;
        CHShapeInternal::ProcessSMotionFrame(lpShape->lpMotion);
    }
}

void Shape_SetFrame(CHShape* lpShape, DWORD dwFrame)
{
    if (!lpShape || !lpShape->lpMotion)
        return;
    
    if (dwFrame < lpShape->lpMotion->dwFrames)
    {
        lpShape->lpMotion->nFrame = static_cast<int>(dwFrame);
        CHShapeInternal::ProcessSMotionFrame(lpShape->lpMotion);
    }
}

void Shape_ClearMatrix(CHShape* lpShape)
{
    if (!lpShape)
        return;
    
    if (lpShape->lpMotion)
    {
        lpShape->lpMotion->matrix = XMMatrixIdentity();
    }
}

BOOL Shape_DrawAlpha(CHShape* lpShape, BOOL bLocal)
{
    if (!lpShape)
        return FALSE;
    
    // Process shape lines and generate geometry
    CHShapeInternal::ProcessShapeLines(lpShape);
    CHShapeInternal::GenerateShapeGeometry(lpShape);
    
    // Render with alpha blending
    return CHShapeInternal::RenderShape(lpShape, false, CH_BLEND_SRCALPHA, CH_BLEND_INVSRCALPHA);
}

// Internal implementation
namespace CHShapeInternal {

void ProcessShapeLines(CHShape* shape)
{
    if (!shape)
        return;
    
    // Process each line in the shape
    for (DWORD i = 0; i < shape->dwLineCount; i++)
    {
        ProcessShapeLine(shape, i);
    }
    
    // Apply smoothing if enabled
    if (shape->dwSmooth > 1)
    {
        SmoothShapeLines(shape);
    }
}

void GenerateShapeGeometry(CHShape* shape)
{
    if (!shape || !shape->vb)
        return;
    
    DWORD vertexIndex = 0;
    
    // Generate vertices for all lines
    for (DWORD lineIdx = 0; lineIdx < shape->dwLineCount; lineIdx++)
    {
        CHLine* line = &shape->lpLine[lineIdx];
        
        for (DWORD i = 0; i < line->dwVecCount - 1 && vertexIndex < shape->dwSegment * 2; i++)
        {
            // First vertex of line segment
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&shape->vb[vertexIndex]), line->lpVB[i]);
            shape->vb[vertexIndex].color = 0xFFFFFFFF;
            shape->vb[vertexIndex].u = static_cast<float>(i) / static_cast<float>(line->dwVecCount - 1);
            shape->vb[vertexIndex].v = 0.0f;
            vertexIndex++;
            
            // Second vertex of line segment
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&shape->vb[vertexIndex]), line->lpVB[i + 1]);
            shape->vb[vertexIndex].color = 0xFFFFFFFF;
            shape->vb[vertexIndex].u = static_cast<float>(i + 1) / static_cast<float>(line->dwVecCount - 1);
            shape->vb[vertexIndex].v = 1.0f;
            vertexIndex++;
        }
    }
    
    shape->dwSegmentCur = vertexIndex / 2;
    
    // Update GPU buffers
    UpdateVertexBuffer(shape);
}

void SmoothShapeLines(CHShape* shape)
{
    if (!shape || !shape->lpSmooths0 || !shape->lpSmooths1)
        return;
    
    // Apply Catmull-Rom spline smoothing to lines
    for (DWORD lineIdx = 0; lineIdx < shape->dwLineCount; lineIdx++)
    {
        CHLine* line = &shape->lpLine[lineIdx];
        if (line->dwVecCount < 4)
            continue; // Need at least 4 points for spline
        
        DWORD smoothCount = (line->dwVecCount - 3) * shape->dwSmooth;
        XMVECTOR* smoothedPoints = new XMVECTOR[smoothCount];
        
        DWORD outputIndex = 0;
        for (DWORD i = 0; i < line->dwVecCount - 3; i++)
        {
            for (DWORD s = 0; s < shape->dwSmooth; s++)
            {
                float t = static_cast<float>(s) / static_cast<float>(shape->dwSmooth);
                smoothedPoints[outputIndex] = InterpolateShapePoint(
                    line->lpVB[i], line->lpVB[i + 1], 
                    line->lpVB[i + 2], line->lpVB[i + 3], t);
                outputIndex++;
            }
        }
        
        // Replace original points with smoothed ones
        delete[] line->lpVB;
        line->lpVB = smoothedPoints;
        line->dwVecCount = smoothCount;
    }
}

XMVECTOR InterpolateShapePoint(XMVECTOR p0, XMVECTOR p1, XMVECTOR p2, XMVECTOR p3, float t)
{
    // Catmull-Rom spline interpolation
    float t2 = t * t;
    float t3 = t2 * t;
    
    XMVECTOR result = XMVectorScale(p0, -0.5f * t3 + t2 - 0.5f * t);
    result = XMVectorAdd(result, XMVectorScale(p1, 1.5f * t3 - 2.5f * t2 + 1.0f));
    result = XMVectorAdd(result, XMVectorScale(p2, -1.5f * t3 + 2.0f * t2 + 0.5f * t));
    result = XMVectorAdd(result, XMVectorScale(p3, 0.5f * t3 - 0.5f * t2));
    
    return result;
}

BOOL CreateVertexBuffer(CHShape* shape)
{
    if (!shape)
        return FALSE;
    
    if (shape->vertexBuffer)
        return TRUE; // Already created
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(CHShapeOutVertex) * shape->dwSegment * 2;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    return SUCCEEDED(g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, shape->vertexBuffer.GetAddressOf()));
}

BOOL CreateIndexBuffer(CHShape* shape)
{
    if (!shape)
        return FALSE;
    
    if (shape->indexBuffer)
        return TRUE; // Already created
    
    // Generate indices for line rendering
    DWORD indexCount = shape->dwSegment * 2;
    WORD* indices = new WORD[indexCount];
    
    for (DWORD i = 0; i < indexCount; i++)
    {
        indices[i] = static_cast<WORD>(i);
    }
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(WORD) * indexCount;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = indices;
    
    BOOL result = SUCCEEDED(g_D3DDevice->CreateBuffer(&bufferDesc, &initData, shape->indexBuffer.GetAddressOf()));
    
    delete[] indices;
    return result;
}

void UpdateVertexBuffer(CHShape* shape)
{
    if (!shape || !shape->vertexBuffer || !shape->vb)
        return;
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (SUCCEEDED(g_D3DContext->Map(shape->vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
        memcpy(mappedResource.pData, shape->vb, sizeof(CHShapeOutVertex) * shape->dwSegmentCur * 2);
        g_D3DContext->Unmap(shape->vertexBuffer.Get(), 0);
    }
}

void ReleaseBuffers(CHShape* shape)
{
    if (!shape)
        return;
    
    shape->vertexBuffer.Reset();
    shape->indexBuffer.Reset();
}

void SetupShapeRenderStates()
{
    // Set render states for shape rendering
    SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
    SetRenderState(CH_RS_SRCBLEND, CH_BLEND_SRCALPHA);
    SetRenderState(CH_RS_DESTBLEND, CH_BLEND_INVSRCALPHA);
    SetRenderState(CH_RS_CULLMODE, CH_CULL_NONE);
    SetRenderState(CH_RS_ZENABLE, TRUE);
}

BOOL RenderShape(CHShape* shape, bool enableZ, int srcBlend, int destBlend)
{
    if (!shape)
        return FALSE;
    
    // Create buffers if needed
    if (!shape->vertexBuffer && !CreateVertexBuffer(shape))
        return FALSE;
    
    if (!shape->indexBuffer && !CreateIndexBuffer(shape))
        return FALSE;
    
    // Set texture if available
    if (shape->nTex >= 0 && shape->nTex < TEX_MAX && g_lpTex[shape->nTex])
    {
        SetTexture(0, g_lpTex[shape->nTex]->lpSRV.Get());
    }
    
    // Set blend modes
    SetRenderState(CH_RS_SRCBLEND, srcBlend);
    SetRenderState(CH_RS_DESTBLEND, destBlend);
    SetRenderState(CH_RS_ZENABLE, enableZ ? TRUE : FALSE);
    
    // Set vertex and index buffers
    g_D3DContext->IASetVertexBuffers(0, 1, shape->vertexBuffer.GetAddressOf(), &shape->vertexStride, &shape->vertexOffset);
    g_D3DContext->IASetIndexBuffer(shape->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    
    // Draw shape
    g_D3DContext->DrawIndexed(shape->dwSegmentCur * 2, 0, 0);
    
    return TRUE;
}

void ProcessShapeLine(CHShape* shape, DWORD lineIndex)
{
    if (!shape || lineIndex >= shape->dwLineCount)
        return;
    
    CHLine* line = &shape->lpLine[lineIndex];
    
    // Apply motion transformation if available
    if (shape->lpMotion && shape->lpMotion->nFrame < static_cast<int>(shape->lpMotion->dwFrames))
    {
        XMMATRIX transform = shape->lpMotion->lpFrames[shape->lpMotion->nFrame];
        transform = XMMatrixMultiply(transform, shape->lpMotion->matrix);
        
        for (DWORD i = 0; i < line->dwVecCount; i++)
        {
            line->lpVB[i] = XMVector3TransformCoord(line->lpVB[i], transform);
        }
    }
}

void OptimizeShapeLines(CHShape* shape)
{
    if (!shape)
        return;
    
    // Remove duplicate consecutive points
    for (DWORD lineIdx = 0; lineIdx < shape->dwLineCount; lineIdx++)
    {
        CHLine* line = &shape->lpLine[lineIdx];
        DWORD writeIndex = 0;
        
        for (DWORD readIndex = 0; readIndex < line->dwVecCount; readIndex++)
        {
            if (readIndex == 0 || !XMVector3NearEqual(line->lpVB[readIndex], line->lpVB[writeIndex - 1], XMVectorSet(0.001f, 0.001f, 0.001f, 0.001f)))
            {
                if (writeIndex != readIndex)
                {
                    line->lpVB[writeIndex] = line->lpVB[readIndex];
                }
                writeIndex++;
            }
        }
        
        line->dwVecCount = writeIndex;
    }
}

BOOL LoadSMotionFromFile(FILE* file, CHSMotion** motion)
{
    // Placeholder for motion loading from file
    return FALSE;
}

BOOL LoadSMotionFromPack(HANDLE handle, CHSMotion** motion)
{
    // Placeholder for motion loading from packed file
    return FALSE;
}

void ProcessSMotionFrame(CHSMotion* motion)
{
    if (!motion || motion->nFrame >= static_cast<int>(motion->dwFrames))
        return;
    
    // Update current matrix from frame data
    motion->matrix = motion->lpFrames[motion->nFrame];
}

BOOL LoadShapeFromFile(FILE* file, CHShape** shape, bool loadTextures)
{
    // Placeholder for shape loading from file
    return FALSE;
}

BOOL LoadShapeFromPack(HANDLE handle, CHShape** shape, bool loadTextures)
{
    // Placeholder for shape loading from packed file
    return FALSE;
}

BOOL SaveShapeToFile(const char* filename, CHShape* shape, bool newFile)
{
    // Placeholder for shape saving to file
    return FALSE;
}

void SetupTearAirEffect(CHShape* shape)
{
    // Placeholder for TearAir effect setup
}

void UpdateTearAirTexture(CHShape* shape)
{
    // Placeholder for TearAir texture update
}

void RenderTearAirEffect(CHShape* shape)
{
    // Placeholder for TearAir effect rendering
}

} // namespace CHShapeInternal