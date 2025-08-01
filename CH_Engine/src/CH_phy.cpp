#include <windows.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "CH_phy.h"
#include "CH_main.h"
#include "CH_texture.h"
#include <algorithm>
#include <algorithm> // for std::min

// Global physics shader manager
namespace CHPhyInternal {
    PhyShaderManager g_PhyShaderManager;
}

void Motion_Clear(CHMotion* lpMotion)
{
    if (!lpMotion)
        return;

    delete[] lpMotion->lpKeyFrame;
    delete[] lpMotion->matrix;
    delete[] lpMotion->lpMorph;

    lpMotion->lpKeyFrame = nullptr;
    lpMotion->matrix = nullptr;
    lpMotion->lpMorph = nullptr;
    lpMotion->dwBoneCount = 0;
    lpMotion->dwFrames = 0;
    lpMotion->dwKeyFrames = 0;
    lpMotion->dwMorphCount = 0;
    lpMotion->nFrame = 0;
}

BOOL Motion_Load(CHMotion** lpMotion, FILE* file)
{
    if (!lpMotion || !file)
        return FALSE;

    *lpMotion = new CHMotion;
    Motion_Clear(*lpMotion);

    // Read motion header
    fread(&(*lpMotion)->dwBoneCount, sizeof(DWORD), 1, file);
    fread(&(*lpMotion)->dwFrames, sizeof(DWORD), 1, file);
    fread(&(*lpMotion)->dwKeyFrames, sizeof(DWORD), 1, file);

    // Allocate and read keyframes
    if ((*lpMotion)->dwKeyFrames > 0)
    {
        (*lpMotion)->lpKeyFrame = new CHKeyFrame[(*lpMotion)->dwKeyFrames];
        for (DWORD i = 0; i < (*lpMotion)->dwKeyFrames; i++)
        {
            fread(&(*lpMotion)->lpKeyFrame[i].pos, sizeof(DWORD), 1, file);

            // Allocate matrix and read as XMFLOAT4X4
            (*lpMotion)->lpKeyFrame[i].matrix = new XMMATRIX;
            XMFLOAT4X4 matrixData;
            fread(&matrixData, sizeof(XMFLOAT4X4), 1, file);
            *(*lpMotion)->lpKeyFrame[i].matrix = XMLoadFloat4x4(&matrixData);
        }
    }

    // Allocate and read bone matrices
    if ((*lpMotion)->dwBoneCount > 0)
    {
        (*lpMotion)->matrix = new XMMATRIX[(*lpMotion)->dwBoneCount];
        for (DWORD i = 0; i < (*lpMotion)->dwBoneCount; i++)
        {
            XMFLOAT4X4 matrixData;
            fread(&matrixData, sizeof(XMFLOAT4X4), 1, file);
            (*lpMotion)->matrix[i] = XMLoadFloat4x4(&matrixData);
        }
    }

    // Read morph data
    fread(&(*lpMotion)->dwMorphCount, sizeof(DWORD), 1, file);
    if ((*lpMotion)->dwMorphCount > 0)
    {
        (*lpMotion)->lpMorph = new float[(*lpMotion)->dwMorphCount];
        fread((*lpMotion)->lpMorph, sizeof(float), (*lpMotion)->dwMorphCount, file);
    }

    fread(&(*lpMotion)->nFrame, sizeof(int), 1, file);

    return TRUE;
}

BOOL Motion_LoadPack(CHMotion** lpMotion, HANDLE f)
{
    if (!lpMotion || f == INVALID_HANDLE_VALUE)
        return FALSE;

    *lpMotion = new CHMotion;
    Motion_Clear(*lpMotion);

    DWORD bytesRead;

    // Read motion data from packed file
    ReadFile(f, &(*lpMotion)->dwBoneCount, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(f, &(*lpMotion)->dwFrames, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(f, &(*lpMotion)->dwKeyFrames, sizeof(DWORD), &bytesRead, nullptr);

    // Read keyframes
    if ((*lpMotion)->dwKeyFrames > 0)
    {
        (*lpMotion)->lpKeyFrame = new CHKeyFrame[(*lpMotion)->dwKeyFrames];
        for (DWORD i = 0; i < (*lpMotion)->dwKeyFrames; i++)
        {
            ReadFile(f, &(*lpMotion)->lpKeyFrame[i].pos, sizeof(DWORD), &bytesRead, nullptr);

            (*lpMotion)->lpKeyFrame[i].matrix = new XMMATRIX;
            XMFLOAT4X4 matrixData;
            ReadFile(f, &matrixData, sizeof(XMFLOAT4X4), &bytesRead, nullptr);
            *(*lpMotion)->lpKeyFrame[i].matrix = XMLoadFloat4x4(&matrixData);
        }
    }

    // Read bone matrices
    if ((*lpMotion)->dwBoneCount > 0)
    {
        (*lpMotion)->matrix = new XMMATRIX[(*lpMotion)->dwBoneCount];
        for (DWORD i = 0; i < (*lpMotion)->dwBoneCount; i++)
        {
            XMFLOAT4X4 matrixData;
            ReadFile(f, &matrixData, sizeof(XMFLOAT4X4), &bytesRead, nullptr);
            (*lpMotion)->matrix[i] = XMLoadFloat4x4(&matrixData);
        }
    }

    // Read morph data
    ReadFile(f, &(*lpMotion)->dwMorphCount, sizeof(DWORD), &bytesRead, nullptr);
    if ((*lpMotion)->dwMorphCount > 0)
    {
        (*lpMotion)->lpMorph = new float[(*lpMotion)->dwMorphCount];
        ReadFile(f, (*lpMotion)->lpMorph, sizeof(float) * (*lpMotion)->dwMorphCount, &bytesRead, nullptr);
    }

    ReadFile(f, &(*lpMotion)->nFrame, sizeof(int), &bytesRead, nullptr);

    return TRUE;
}

void Phy_Clear(CHPhy* lpPhy)
{
    if (!lpPhy)
        return;

    delete[] lpPhy->lpName;
    delete[] lpPhy->lpVB;
    delete[] lpPhy->lpIB;
    delete[] lpPhy->lpOutVB;
    delete[] lpPhy->lpTexName;

    lpPhy->lpName = nullptr;
    lpPhy->lpVB = nullptr;
    lpPhy->lpIB = nullptr;
    lpPhy->lpOutVB = nullptr;
    lpPhy->lpTexName = nullptr;

    if (lpPhy->lpMotion)
    {
        Motion_Unload(&lpPhy->lpMotion);
    }

    Key_Clear(&lpPhy->Key);
    CHPhyInternal::ReleaseBuffers(lpPhy);

    lpPhy->dwBlendCount = 0;
    lpPhy->dwNVecCount = 0;
    lpPhy->dwAVecCount = 0;
    lpPhy->dwNTriCount = 0;
    lpPhy->dwATriCount = 0;
    lpPhy->nTex = -1;
    lpPhy->nTex2 = -1;
    lpPhy->bDraw = TRUE;
    lpPhy->dwTexRow = 0;
    lpPhy->fA = 1.0f;
    lpPhy->fR = 1.0f;
    lpPhy->fG = 1.0f;
    lpPhy->fB = 1.0f;
    lpPhy->uvstep = XMFLOAT2(0.0f, 0.0f);
    lpPhy->InitMatrix = XMMatrixIdentity();
    lpPhy->bboxMin = XMVectorZero();
    lpPhy->bboxMax = XMVectorZero();
    lpPhy->normalVertexStride = sizeof(CHPhyOutVertex);
    lpPhy->alphaVertexStride = sizeof(CHPhyOutVertex);
    lpPhy->vertexOffset = 0;
}

BOOL Phy_Load(CHPhy** lpPhy, FILE* file, BOOL bTex)
{
    if (!lpPhy || !file)
        return FALSE;

    *lpPhy = new CHPhy;
    Phy_Clear(*lpPhy);

    // Read version
    char version[17];
    fread(version, 16, 1, file);
    version[16] = '\0';

    if (strcmp(version, CH_VERSION) != 0)
    {
        delete* lpPhy;
        *lpPhy = nullptr;
        return FALSE;
    }

    ChunkHeader chunk;
    while (fread(&chunk, sizeof(ChunkHeader), 1, file) == 1)
    {
        if (chunk.byChunkID[0] == 'P' && chunk.byChunkID[1] == 'H' &&
            chunk.byChunkID[2] == 'Y' && chunk.byChunkID[3] == 'S')
        {
            // Read phy data
            DWORD nameLen;
            fread(&nameLen, sizeof(DWORD), 1, file);
            if (nameLen > 0)
            {
                (*lpPhy)->lpName = new char[nameLen + 1];
                fread((*lpPhy)->lpName, nameLen, 1, file);
                (*lpPhy)->lpName[nameLen] = '\0';
            }

            // Read vertex and triangle counts
            fread(&(*lpPhy)->dwBlendCount, sizeof(DWORD), 1, file);
            fread(&(*lpPhy)->dwNVecCount, sizeof(DWORD), 1, file);
            fread(&(*lpPhy)->dwAVecCount, sizeof(DWORD), 1, file);
            fread(&(*lpPhy)->dwNTriCount, sizeof(DWORD), 1, file);
            fread(&(*lpPhy)->dwATriCount, sizeof(DWORD), 1, file);

            // Read vertices
            DWORD totalVerts = (*lpPhy)->dwNVecCount + (*lpPhy)->dwAVecCount;
            if (totalVerts > 0)
            {
                (*lpPhy)->lpVB = new CHPhyVertex[totalVerts];
                fread((*lpPhy)->lpVB, sizeof(CHPhyVertex), totalVerts, file);
            }

            // Read indices
            DWORD totalIndices = ((*lpPhy)->dwNTriCount + (*lpPhy)->dwATriCount) * 3;
            if (totalIndices > 0)
            {
                (*lpPhy)->lpIB = new WORD[totalIndices];
                fread((*lpPhy)->lpIB, sizeof(WORD), totalIndices, file);
            }

            // Read texture name
            fread(&nameLen, sizeof(DWORD), 1, file);
            if (nameLen > 0 && bTex)
            {
                (*lpPhy)->lpTexName = new char[nameLen + 1];
                fread((*lpPhy)->lpTexName, nameLen, 1, file);
                (*lpPhy)->lpTexName[nameLen] = '\0';

                // Load texture
                CHTexture* tex;
                (*lpPhy)->nTex = Texture_Load(&tex, (*lpPhy)->lpTexName);
            }

            // Read bounding box
            XMFLOAT3 bboxMinFloat, bboxMaxFloat;
            fread(&bboxMinFloat, sizeof(XMFLOAT3), 1, file);
            fread(&bboxMaxFloat, sizeof(XMFLOAT3), 1, file);
            (*lpPhy)->bboxMin = XMLoadFloat3(&bboxMinFloat);
            (*lpPhy)->bboxMax = XMLoadFloat3(&bboxMaxFloat);

            // Read initial matrix
            XMFLOAT4X4 initMatrix;
            fread(&initMatrix, sizeof(XMFLOAT4X4), 1, file);
            (*lpPhy)->InitMatrix = XMLoadFloat4x4(&initMatrix);

            // Create output vertex buffer
            (*lpPhy)->lpOutVB = new CHPhyOutVertex[totalVerts];

            // Create DirectX 11 buffers
            CHPhyInternal::CreateVertexBuffers(*lpPhy);
            CHPhyInternal::CreateIndexBuffers(*lpPhy);
            CHPhyInternal::CreateBoneMatrixBuffer(*lpPhy);

            break;
        }
        else
        {
            fseek(file, chunk.dwChunkSize, SEEK_CUR);
        }
    }

    return TRUE;
}

BOOL Phy_Calculate(CHPhy* lpPhy)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return FALSE;

    // Process animation keys
    float alpha;
    if (Key_ProcessAlpha(&lpPhy->Key, lpPhy->lpMotion->nFrame,
        lpPhy->lpMotion->dwFrames, &alpha))
        lpPhy->fA = alpha;

    BOOL draw;
    if (Key_ProcessDraw(&lpPhy->Key, lpPhy->lpMotion->nFrame, &draw))
        lpPhy->bDraw = draw;

    int tex = -1;
    Key_ProcessChangeTex(&lpPhy->Key, lpPhy->lpMotion->nFrame, &tex);

    if (!lpPhy->bDraw)
        return TRUE;

    // Process skeletal animation
    CHPhyInternal::ProcessMotionKeyframes(lpPhy->lpMotion);
    CHPhyInternal::ProcessVertexBlending(lpPhy);
    CHPhyInternal::UpdateVertexBuffer(lpPhy, false); // Normal vertices
    CHPhyInternal::UpdateVertexBuffer(lpPhy, true);  // Alpha vertices

    return TRUE;
}

BOOL Phy_DrawNormal(CHPhy* lpPhy)
{
    if (!lpPhy || !lpPhy->bDraw || lpPhy->dwNTriCount == 0)
        return FALSE;

    return CHPhyInternal::RenderNormalMesh(lpPhy);
}

BOOL Phy_DrawAlpha(CHPhy* lpPhy, BOOL bZ, int nAsb, int nAdb)
{
    if (!lpPhy || !lpPhy->bDraw || lpPhy->dwATriCount == 0)
        return FALSE;

    return CHPhyInternal::RenderAlphaMesh(lpPhy, bZ != FALSE, nAsb, nAdb);
}

void Phy_NextFrame(CHPhy* lpPhy, int nStep)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return;

    if (lpPhy->lpMotion->dwFrames > 0)
    {
        lpPhy->lpMotion->nFrame = (lpPhy->lpMotion->nFrame + nStep) % lpPhy->lpMotion->dwFrames;
    }
}

void Phy_SetFrame(CHPhy* lpPhy, DWORD dwFrame)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return;

    if (dwFrame < lpPhy->lpMotion->dwFrames)
    {
        lpPhy->lpMotion->nFrame = static_cast<int>(dwFrame);
    }
}

// Internal implementation
namespace CHPhyInternal {

    void ProcessVertexBlending(CHPhy* phy)
    {
        if (!phy || !phy->lpVB || !phy->lpOutVB || !phy->lpMotion)
            return;

        DWORD totalVerts = phy->dwNVecCount + phy->dwAVecCount;

        for (DWORD i = 0; i < totalVerts; i++)
        {
            CHPhyVertex* srcVert = &phy->lpVB[i];
            CHPhyOutVertex* outVert = &phy->lpOutVB[i];

            // Initialize with first morph target
            XMVECTOR blendedPos = srcVert->pos[0];

            // Apply morph targets if available
            if (phy->lpMotion->dwMorphCount > 0 && phy->lpMotion->lpMorph)
            {
                blendedPos = XMVectorZero();
                for (DWORD m = 0; m < CH_MORPH_MAX && m < phy->lpMotion->dwMorphCount; m++)
                {
                    float weight = (m < phy->lpMotion->dwMorphCount) ? phy->lpMotion->lpMorph[m] : 0.0f;
                    blendedPos = XMVectorAdd(blendedPos, XMVectorScale(srcVert->pos[m], weight));
                }
            }

            // Apply bone transformations
            XMVECTOR finalPos = XMVectorZero();
            float totalWeight = 0.0f;

            for (DWORD b = 0; b < CH_BONE_MAX; b++)
            {
                if (srcVert->weight[b] > 0.0f && srcVert->index[b] < phy->lpMotion->dwBoneCount)
                {
                    XMMATRIX boneMatrix = phy->lpMotion->matrix[srcVert->index[b]];
                    XMVECTOR transformedPos = XMVector3TransformCoord(blendedPos, boneMatrix);
                    finalPos = XMVectorAdd(finalPos, XMVectorScale(transformedPos, srcVert->weight[b]));
                    totalWeight += srcVert->weight[b];
                }
            }

            // Normalize if weights don't sum to 1
            if (totalWeight > 0.0f && totalWeight != 1.0f)
            {
                finalPos = XMVectorScale(finalPos, 1.0f / totalWeight);
            }
            else if (totalWeight == 0.0f)
            {
                finalPos = blendedPos; // No bone influence
            }

            // Store final position
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&outVert->x), finalPos);

            // Copy other vertex data
            outVert->color = srcVert->color;
            outVert->u = srcVert->u;
            outVert->v = srcVert->v;

            // Apply color modulation
            DWORD r = ((outVert->color >> 16) & 0xFF);
            DWORD g = ((outVert->color >> 8) & 0xFF);
            DWORD b = (outVert->color & 0xFF);
            DWORD a = ((outVert->color >> 24) & 0xFF);

            r = static_cast<DWORD>(r * phy->fR);
            g = static_cast<DWORD>(g * phy->fG);
            b = static_cast<DWORD>(b * phy->fB);
            a = static_cast<DWORD>(a * phy->fA);

                         r = std::min(255UL, r);
             g = std::min(255UL, g);
             b = std::min(255UL, b);
             a = std::min(255UL, a);

            outVert->color = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    void ProcessMotionKeyframes(CHMotion* motion)
    {
        if (!motion || !motion->lpKeyFrame || motion->dwKeyFrames == 0)
            return;

        // Find keyframes around current frame
        int currentFrame = motion->nFrame % motion->dwFrames;
        CHKeyFrame* prevKeyframe = nullptr;
        CHKeyFrame* nextKeyframe = nullptr;

        for (DWORD i = 0; i < motion->dwKeyFrames; i++)
        {
            if (motion->lpKeyFrame[i].pos <= currentFrame)
            {
                prevKeyframe = &motion->lpKeyFrame[i];
            }
            if (motion->lpKeyFrame[i].pos > currentFrame && !nextKeyframe)
            {
                nextKeyframe = &motion->lpKeyFrame[i];
                break;
            }
        }

        // Apply keyframe transformation to bone matrices
        if (prevKeyframe && prevKeyframe->matrix)
        {
            for (DWORD i = 0; i < motion->dwBoneCount; i++)
            {
                if (nextKeyframe && nextKeyframe->matrix)
                {
                    // Interpolate between keyframes
                    float t = static_cast<float>(currentFrame - prevKeyframe->pos) /
                        static_cast<float>(nextKeyframe->pos - prevKeyframe->pos);

                    // Simple linear interpolation (could be improved with quaternion slerp)
                    XMMATRIX prevMatrix = XMMatrixMultiply(*prevKeyframe->matrix,
                        XMMatrixScaling(1.0f - t, 1.0f - t, 1.0f - t));
                    XMMATRIX nextMatrix = XMMatrixMultiply(*nextKeyframe->matrix, 
                        XMMatrixScaling(t, t, t));
                    
                    // Combine matrices (DirectX Math doesn't have XMMatrixAdd)
                    motion->matrix[i] = XMMatrixMultiply(motion->matrix[i], 
                        XMMatrixMultiply(prevMatrix, nextMatrix));
                }
                else
                {
                    // Use single keyframe
                    motion->matrix[i] = XMMatrixMultiply(motion->matrix[i], *prevKeyframe->matrix);
                }
            }
        }
    }

    BOOL CreateVertexBuffers(CHPhy* phy)
    {
        if (!phy)
            return FALSE;

        DWORD normalVertCount = phy->dwNVecCount;
        DWORD alphaVertCount = phy->dwAVecCount;

        // Create normal vertex buffer
        if (normalVertCount > 0)
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.ByteWidth = sizeof(CHPhyOutVertex) * normalVertCount;
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            if (FAILED(g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, phy->normalVertexBuffer.GetAddressOf())))
                return FALSE;
        }

        // Create alpha vertex buffer
        if (alphaVertCount > 0)
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.ByteWidth = sizeof(CHPhyOutVertex) * alphaVertCount;
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            if (FAILED(g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, phy->alphaVertexBuffer.GetAddressOf())))
                return FALSE;
        }

        return TRUE;
    }

    BOOL CreateIndexBuffers(CHPhy* phy)
    {
        if (!phy)
            return FALSE;

        DWORD normalIndexCount = phy->dwNTriCount * 3;
        DWORD alphaIndexCount = phy->dwATriCount * 3;

        // Create normal index buffer
        if (normalIndexCount > 0)
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            bufferDesc.ByteWidth = sizeof(WORD) * normalIndexCount;
            bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = phy->lpIB;

            if (FAILED(g_D3DDevice->CreateBuffer(&bufferDesc, &initData, phy->normalIndexBuffer.GetAddressOf())))
                return FALSE;
        }

        // Create alpha index buffer
        if (alphaIndexCount > 0)
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            bufferDesc.ByteWidth = sizeof(WORD) * alphaIndexCount;
            bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = phy->lpIB + normalIndexCount;

            if (FAILED(g_D3DDevice->CreateBuffer(&bufferDesc, &initData, phy->alphaIndexBuffer.GetAddressOf())))
                return FALSE;
        }

        return TRUE;
    }

    BOOL CreateBoneMatrixBuffer(CHPhy* phy)
    {
        if (!phy || !phy->lpMotion)
            return FALSE;

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.ByteWidth = sizeof(XMMATRIX) * phy->lpMotion->dwBoneCount;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        return SUCCEEDED(g_D3DDevice->CreateBuffer(&bufferDesc, nullptr, phy->boneMatrixBuffer.GetAddressOf()));
    }

    void UpdateVertexBuffer(CHPhy* phy, bool isAlpha)
    {
        if (!phy || !phy->lpOutVB)
            return;

        CHComPtr<ID3D11Buffer> buffer = isAlpha ? phy->alphaVertexBuffer : phy->normalVertexBuffer;
        if (!buffer)
            return;

        DWORD vertCount = isAlpha ? phy->dwAVecCount : phy->dwNVecCount;
        DWORD vertOffset = isAlpha ? phy->dwNVecCount : 0;

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(g_D3DContext->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        {
            memcpy(mappedResource.pData, phy->lpOutVB + vertOffset, sizeof(CHPhyOutVertex) * vertCount);
            g_D3DContext->Unmap(buffer.Get(), 0);
        }
    }

    void ReleaseBuffers(CHPhy* phy)
    {
        if (!phy)
            return;

        phy->normalVertexBuffer.Reset();
        phy->alphaVertexBuffer.Reset();
        phy->normalIndexBuffer.Reset();
        phy->alphaIndexBuffer.Reset();
        phy->boneMatrixBuffer.Reset();
    }

    BOOL RenderNormalMesh(CHPhy* phy)
    {
        if (!phy || !phy->normalVertexBuffer || !phy->normalIndexBuffer)
            return FALSE;

        // Set texture
        if (phy->nTex >= 0 && phy->nTex < TEX_MAX && g_lpTex[phy->nTex])
        {
            SetTexture(0, g_lpTex[phy->nTex]->lpSRV.Get());
        }

        // Set render states for normal mesh
        SetRenderState(CH_RS_ALPHABLENDENABLE, FALSE);
        SetRenderState(CH_RS_ZENABLE, TRUE);
        SetRenderState(CH_RS_ZWRITEENABLE, TRUE);

        // Set vertex and index buffers
        g_D3DContext->IASetVertexBuffers(0, 1, phy->normalVertexBuffer.GetAddressOf(),
            &phy->normalVertexStride, &phy->vertexOffset);
        g_D3DContext->IASetIndexBuffer(phy->normalIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set shaders
        g_PhyShaderManager.SetSkeletalShaders();

        // Draw
        g_D3DContext->DrawIndexed(phy->dwNTriCount * 3, 0, 0);

        return TRUE;
    }

    BOOL RenderAlphaMesh(CHPhy* phy, bool enableZ, int srcBlend, int destBlend)
    {
        if (!phy || !phy->alphaVertexBuffer || !phy->alphaIndexBuffer)
            return FALSE;

        // Set texture
        if (phy->nTex >= 0 && phy->nTex < TEX_MAX && g_lpTex[phy->nTex])
        {
            SetTexture(0, g_lpTex[phy->nTex]->lpSRV.Get());
        }

        // Set render states for alpha mesh
        SetRenderState(CH_RS_ALPHABLENDENABLE, TRUE);
        SetRenderState(CH_RS_SRCBLEND, srcBlend);
        SetRenderState(CH_RS_DESTBLEND, destBlend);
        SetRenderState(CH_RS_ZWRITEENABLE, enableZ ? TRUE : FALSE);

        // Set vertex and index buffers
        g_D3DContext->IASetVertexBuffers(0, 1, phy->alphaVertexBuffer.GetAddressOf(),
            &phy->alphaVertexStride, &phy->vertexOffset);
        g_D3DContext->IASetIndexBuffer(phy->alphaIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        g_D3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set shaders
        g_PhyShaderManager.SetSkeletalShaders();

        // Draw
        g_D3DContext->DrawIndexed(phy->dwATriCount * 3, 0, 0);

        return TRUE;
    }

    void SetSkeletalShaders()
    {
        // Use the shader manager's method instead of direct access
        g_PhyShaderManager.SetSkeletalShaders();
    }

    // Shader initialization is handled by PhyShaderManager class

} // namespace CHPhyInternal

// Additional motion functions
CH_CORE_DLL_API
void Motion_Unload(CHMotion** lpMotion)
{
    if (!lpMotion || !*lpMotion)
        return;

    Motion_Clear(*lpMotion);
    delete* lpMotion;
    *lpMotion = nullptr;
}

CH_CORE_DLL_API
BOOL Motion_Save(char* lpName, CHMotion* lpMotion, BOOL bNew)
{
    if (!lpName || !lpMotion)
        return FALSE;

    FILE* file = fopen(lpName, bNew ? "wb" : "ab");
    if (!file)
        return FALSE;

    // Write motion header
    fwrite(&lpMotion->dwBoneCount, sizeof(DWORD), 1, file);
    fwrite(&lpMotion->dwFrames, sizeof(DWORD), 1, file);
    fwrite(&lpMotion->dwKeyFrames, sizeof(DWORD), 1, file);

    // Write keyframes
    for (DWORD i = 0; i < lpMotion->dwKeyFrames; i++)
    {
        fwrite(&lpMotion->lpKeyFrame[i].pos, sizeof(DWORD), 1, file);
        
        // Convert XMMATRIX to XMFLOAT4X4 for storage
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, *lpMotion->lpKeyFrame[i].matrix);
        fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
    }

    // Write bone matrices
    for (DWORD i = 0; i < lpMotion->dwBoneCount; i++)
    {
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, lpMotion->matrix[i]);
        fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
    }

    // Write morph data
    fwrite(&lpMotion->dwMorphCount, sizeof(DWORD), 1, file);
    if (lpMotion->dwMorphCount > 0)
    {
        fwrite(lpMotion->lpMorph, sizeof(float), lpMotion->dwMorphCount, file);
    }

    fwrite(&lpMotion->nFrame, sizeof(int), 1, file);

    fclose(file);
    return TRUE;
}

CH_CORE_DLL_API
void Motion_GetMatrix(CHMotion* lpMotion, DWORD dwBone, XMMATRIX* lpMatrix)
{
    if (!lpMotion || !lpMatrix || dwBone >= lpMotion->dwBoneCount)
        return;

    *lpMatrix = lpMotion->matrix[dwBone];
}

// Additional physics functions
CH_CORE_DLL_API
BOOL Phy_LoadPack(CHPhy** lpPhy, HANDLE f, BOOL bTex)
{
    if (!lpPhy || f == INVALID_HANDLE_VALUE)
        return FALSE;

    *lpPhy = new CHPhy;
    Phy_Clear(*lpPhy);

    DWORD bytesRead;

    // Read version
    char version[17];
    ReadFile(f, version, 16, &bytesRead, nullptr);
    version[16] = '\0';

    if (strcmp(version, CH_VERSION) != 0)
    {
        delete* lpPhy;
        *lpPhy = nullptr;
        return FALSE;
    }

    // Read phy data
    DWORD nameLen;
    ReadFile(f, &nameLen, sizeof(DWORD), &bytesRead, nullptr);
    if (nameLen > 0)
    {
        (*lpPhy)->lpName = new char[nameLen + 1];
        ReadFile(f, (*lpPhy)->lpName, nameLen, &bytesRead, nullptr);
        (*lpPhy)->lpName[nameLen] = '\0';
    }

    // Read vertex and triangle counts
    ReadFile(f, &(*lpPhy)->dwBlendCount, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(f, &(*lpPhy)->dwNVecCount, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(f, &(*lpPhy)->dwAVecCount, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(f, &(*lpPhy)->dwNTriCount, sizeof(DWORD), &bytesRead, nullptr);
    ReadFile(f, &(*lpPhy)->dwATriCount, sizeof(DWORD), &bytesRead, nullptr);

    // Read vertices
    DWORD totalVerts = (*lpPhy)->dwNVecCount + (*lpPhy)->dwAVecCount;
    if (totalVerts > 0)
    {
        (*lpPhy)->lpVB = new CHPhyVertex[totalVerts];
        ReadFile(f, (*lpPhy)->lpVB, sizeof(CHPhyVertex) * totalVerts, &bytesRead, nullptr);
    }

    // Read indices
    DWORD totalIndices = ((*lpPhy)->dwNTriCount + (*lpPhy)->dwATriCount) * 3;
    if (totalIndices > 0)
    {
        (*lpPhy)->lpIB = new WORD[totalIndices];
        ReadFile(f, (*lpPhy)->lpIB, sizeof(WORD) * totalIndices, &bytesRead, nullptr);
    }

    // Read texture name
    ReadFile(f, &nameLen, sizeof(DWORD), &bytesRead, nullptr);
    if (nameLen > 0 && bTex)
    {
        (*lpPhy)->lpTexName = new char[nameLen + 1];
        ReadFile(f, (*lpPhy)->lpTexName, nameLen, &bytesRead, nullptr);
        (*lpPhy)->lpTexName[nameLen] = '\0';

        // Load texture
        CHTexture* tex;
        (*lpPhy)->nTex = Texture_Load(&tex, (*lpPhy)->lpTexName);
    }

    // Read bounding box
    XMFLOAT3 bboxMinFloat, bboxMaxFloat;
    ReadFile(f, &bboxMinFloat, sizeof(XMFLOAT3), &bytesRead, nullptr);
    ReadFile(f, &bboxMaxFloat, sizeof(XMFLOAT3), &bytesRead, nullptr);
    (*lpPhy)->bboxMin = XMLoadFloat3(&bboxMinFloat);
    (*lpPhy)->bboxMax = XMLoadFloat3(&bboxMaxFloat);

    // Read initial matrix
    XMFLOAT4X4 initMatrix;
    ReadFile(f, &initMatrix, sizeof(XMFLOAT4X4), &bytesRead, nullptr);
    (*lpPhy)->InitMatrix = XMLoadFloat4x4(&initMatrix);

    // Create output vertex buffer
    (*lpPhy)->lpOutVB = new CHPhyOutVertex[totalVerts];

    // Create DirectX 11 buffers
    CHPhyInternal::CreateVertexBuffers(*lpPhy);
    CHPhyInternal::CreateIndexBuffers(*lpPhy);
    CHPhyInternal::CreateBoneMatrixBuffer(*lpPhy);

    return TRUE;
}

CH_CORE_DLL_API
BOOL Phy_Save(char* lpName, CHPhy* lpPhy, BOOL bNew)
{
    if (!lpName || !lpPhy)
        return FALSE;

    FILE* file = fopen(lpName, bNew ? "wb" : "ab");
    if (!file)
        return FALSE;

    // Write version
    fwrite(CH_VERSION, 16, 1, file);

    // Write chunk header
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'P';
    chunk.byChunkID[1] = 'H';
    chunk.byChunkID[2] = 'Y';
    chunk.byChunkID[3] = 'S';
    chunk.dwChunkSize = 0; // Will be calculated later
    fwrite(&chunk, sizeof(ChunkHeader), 1, file);

    // Write phy data
    DWORD nameLen = lpPhy->lpName ? strlen(lpPhy->lpName) : 0;
    fwrite(&nameLen, sizeof(DWORD), 1, file);
    if (nameLen > 0)
    {
        fwrite(lpPhy->lpName, nameLen, 1, file);
    }

    // Write vertex and triangle counts
    fwrite(&lpPhy->dwBlendCount, sizeof(DWORD), 1, file);
    fwrite(&lpPhy->dwNVecCount, sizeof(DWORD), 1, file);
    fwrite(&lpPhy->dwAVecCount, sizeof(DWORD), 1, file);
    fwrite(&lpPhy->dwNTriCount, sizeof(DWORD), 1, file);
    fwrite(&lpPhy->dwATriCount, sizeof(DWORD), 1, file);

    // Write vertices
    DWORD totalVerts = lpPhy->dwNVecCount + lpPhy->dwAVecCount;
    if (totalVerts > 0)
    {
        fwrite(lpPhy->lpVB, sizeof(CHPhyVertex), totalVerts, file);
    }

    // Write indices
    DWORD totalIndices = (lpPhy->dwNTriCount + lpPhy->dwATriCount) * 3;
    if (totalIndices > 0)
    {
        fwrite(lpPhy->lpIB, sizeof(WORD), totalIndices, file);
    }

    // Write texture name
    nameLen = lpPhy->lpTexName ? strlen(lpPhy->lpTexName) : 0;
    fwrite(&nameLen, sizeof(DWORD), 1, file);
    if (nameLen > 0)
    {
        fwrite(lpPhy->lpTexName, nameLen, 1, file);
    }

    // Write bounding box
    XMFLOAT3 bboxMinFloat, bboxMaxFloat;
    XMStoreFloat3(&bboxMinFloat, lpPhy->bboxMin);
    XMStoreFloat3(&bboxMaxFloat, lpPhy->bboxMax);
    fwrite(&bboxMinFloat, sizeof(XMFLOAT3), 1, file);
    fwrite(&bboxMaxFloat, sizeof(XMFLOAT3), 1, file);

    // Write initial matrix
    XMFLOAT4X4 initMatrix;
    XMStoreFloat4x4(&initMatrix, lpPhy->InitMatrix);
    fwrite(&initMatrix, sizeof(XMFLOAT4X4), 1, file);

    fclose(file);
    return TRUE;
}

CH_CORE_DLL_API
void Phy_Unload(CHPhy** lpPhy)
{
    if (!lpPhy || !*lpPhy)
        return;

    Phy_Clear(*lpPhy);
    delete* lpPhy;
    *lpPhy = nullptr;
}

CH_CORE_DLL_API
void Phy_Prepare()
{
    // Initialize shaders if not already done
    CHPhyInternal::g_PhyShaderManager.Initialize();
}

CH_CORE_DLL_API
void Phy_Muliply(CHPhy* lpPhy, int nBoneIndex, XMMATRIX* matrix)
{
    if (!lpPhy || !lpPhy->lpMotion || !matrix)
        return;

    int start, end;
    if (nBoneIndex == -1)
    {
        start = 0;
        end = lpPhy->lpMotion->dwBoneCount;
    }
    else
    {
        start = nBoneIndex;
        end = start + 1;
    }

    for (int n = start; n < end; n++)
    {
        lpPhy->lpMotion->matrix[n] = XMMatrixMultiply(lpPhy->lpMotion->matrix[n], *matrix);
    }
}

CH_CORE_DLL_API
void Phy_SetColor(CHPhy* lpPhy,
                  float alpha,
                  float red,
                  float green,
                  float blue)
{
    if (!lpPhy)
        return;

    lpPhy->fA = alpha;
    lpPhy->fR = red;
    lpPhy->fG = green;
    lpPhy->fB = blue;
}

CH_CORE_DLL_API
void Phy_ClearMatrix(CHPhy* lpPhy)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return;

    // Reset bone matrices to identity
    for (DWORD n = 0; n < lpPhy->lpMotion->dwBoneCount; n++)
    {
        lpPhy->lpMotion->matrix[n] = XMMatrixIdentity();
    }
}

CH_CORE_DLL_API
void Phy_ChangeTexture(CHPhy* lpPhy, int nTexID, int nTexID2)
{
    if (!lpPhy)
        return;

    lpPhy->nTex = nTexID;
    lpPhy->nTex2 = nTexID2;
}