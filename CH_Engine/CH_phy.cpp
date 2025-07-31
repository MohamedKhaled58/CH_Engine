#include "CH_phy.h"
#include "CH_main.h"
#include "CH_texture.h"

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
    
    return CHPhyInternal::LoadMotionFromFile(file, lpMotion);
}

BOOL Motion_LoadPack(CHMotion** lpMotion, HANDLE f)
{
    if (!lpMotion || f == INVALID_HANDLE_VALUE)
        return FALSE;
    
    return CHPhyInternal::LoadMotionFromPack(f, lpMotion);
}

BOOL Motion_Save(char* lpName, CHMotion* lpMotion, BOOL bNew)
{
    if (!lpName || !lpMotion)
        return FALSE;
    
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);
    
    // Motion chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'M';
    chunk.byChunkID[1] = 'O';
    chunk.byChunkID[2] = 'T';
    chunk.byChunkID[3] = 'I';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);
    
    // Bone count
    fwrite(&lpMotion->dwBoneCount, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    
    // Frame count  
    fwrite(&lpMotion->dwFrames, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    
    // Keyframes
    fwrite(&lpMotion->dwKeyFrames, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    
    for (DWORD i = 0; i < lpMotion->dwKeyFrames; i++)
    {
        fwrite(&lpMotion->lpKeyFrame[i].pos, sizeof(DWORD), 1, file);
        chunk.dwChunkSize += sizeof(DWORD);
        
        // Convert XMMATRIX to float array for file storage
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, *lpMotion->lpKeyFrame[i].matrix);
        fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
        chunk.dwChunkSize += sizeof(XMFLOAT4X4);
    }
    
    // Bone matrices (convert XMMATRIX to float arrays)
    for (DWORD i = 0; i < lpMotion->dwBoneCount; i++)
    {
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, lpMotion->matrix[i]);
        fwrite(&matrixData, sizeof(XMFLOAT4X4), 1, file);
        chunk.dwChunkSize += sizeof(XMFLOAT4X4);
    }
    
    // Morph data
    fwrite(&lpMotion->dwMorphCount, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    
    if (lpMotion->dwMorphCount > 0)
    {
        fwrite(lpMotion->lpMorph, sizeof(float) * lpMotion->dwMorphCount, 1, file);
        chunk.dwChunkSize += sizeof(float) * lpMotion->dwMorphCount;
    }
    
    // Current frame
    fwrite(&lpMotion->nFrame, sizeof(int), 1, file);
    chunk.dwChunkSize += sizeof(int);
    
    // Update chunk size
    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);
    
    fclose(file);
    return TRUE;
}

void Motion_Unload(CHMotion** lpMotion)
{
    if (!lpMotion || !*lpMotion)
        return;
    
    Motion_Clear(*lpMotion);
    delete *lpMotion;
    *lpMotion = nullptr;
}

void Motion_GetMatrix(CHMotion* lpMotion, DWORD dwBone, XMMATRIX* lpMatrix)
{
    if (!lpMotion || !lpMatrix || dwBone >= lpMotion->dwBoneCount)
        return;
    
    *lpMatrix = lpMotion->matrix[dwBone];
}

void Phy_Clear(CHPhy* lpPhy)
{
    if (!lpPhy)
        return;
        
    // Clear name
    delete[] lpPhy->lpName;
    lpPhy->lpName = nullptr;
    
    // Clear vertex data
    delete[] lpPhy->lpVB;
    delete[] lpPhy->lpIB;
    delete[] lpPhy->lpOutVB;
    lpPhy->lpVB = nullptr;
    lpPhy->lpIB = nullptr;
    lpPhy->lpOutVB = nullptr;
    
    // Clear texture name
    delete[] lpPhy->lpTexName;
    lpPhy->lpTexName = nullptr;
    
    // Clear motion
    if (lpPhy->lpMotion)
    {
        Motion_Unload(&lpPhy->lpMotion);
    }
    
    // Clear keys
    Key_Clear(&lpPhy->Key);
    
    // Clear DirectX 11 buffers
    CHPhyInternal::ReleaseBuffers(lpPhy);
    
    // Reset counters
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
    
    return CHPhyInternal::LoadPhyFromFile(file, lpPhy, bTex != FALSE);
}

BOOL Phy_LoadPack(CHPhy** lpPhy, HANDLE f, BOOL bTex)
{
    if (!lpPhy || f == INVALID_HANDLE_VALUE)
        return FALSE;
    
    return CHPhyInternal::LoadPhyFromPack(f, lpPhy, bTex != FALSE);
}

BOOL Phy_Save(char* lpName, CHPhy* lpPhy, BOOL bNew)
{
    if (!lpName || !lpPhy)
        return FALSE;
    
    return CHPhyInternal::SavePhyToFile(lpName, lpPhy, bNew != FALSE);
}

void Phy_Unload(CHPhy** lpPhy)
{
    if (!lpPhy || !*lpPhy)
        return;
    
    Phy_Clear(*lpPhy);
    delete *lpPhy;
    *lpPhy = nullptr;
}

void Phy_Prepare()
{
    CHPhyInternal::SetupPhyRenderStates();
}

BOOL Phy_Calculate(CHPhy* lpPhy)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return FALSE;

    // Process animation keys (exact same logic as original)
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

    // Process skeletal animation (DX11 implementation)
    CHPhyInternal::ProcessVertexBlending(lpPhy);
    CHPhyInternal::ApplyBoneTransforms(lpPhy);
    CHPhyInternal::ApplyMorphTargets(lpPhy);
    CHPhyInternal::UpdateVertexBuffer(lpPhy, false); // Normal vertices
    CHPhyInternal::UpdateVertexBuffer(lpPhy, true);  // Alpha vertices

    return TRUE;
}

BOOL Phy_DrawNormal(CHPhy* lpPhy)
{
    if (!lpPhy || !lpPhy->bDraw)
        return FALSE;
    
    return CHPhyInternal::RenderNormalMesh(lpPhy);
}

BOOL Phy_DrawAlpha(CHPhy* lpPhy, BOOL bZ, int nAsb, int nAdb)
{
    if (!lpPhy || !lpPhy->bDraw)
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
        CHPhyInternal::ProcessMotionKeyframes(lpPhy->lpMotion);
    }
}

void Phy_SetFrame(CHPhy* lpPhy, DWORD dwFrame)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return;
    
    if (dwFrame < lpPhy->lpMotion->dwFrames)
    {
        lpPhy->lpMotion->nFrame = static_cast<int>(dwFrame);
        CHPhyInternal::ProcessMotionKeyframes(lpPhy->lpMotion);
    }
}

void Phy_Muliply(CHPhy* lpPhy, int nBoneIndex, XMMATRIX* matrix)
{
    if (!lpPhy || !matrix || !lpPhy->lpMotion)
        return;
    
    if (nBoneIndex >= 0 && nBoneIndex < static_cast<int>(lpPhy->lpMotion->dwBoneCount))
    {
        lpPhy->lpMotion->matrix[nBoneIndex] = XMMatrixMultiply(lpPhy->lpMotion->matrix[nBoneIndex], *matrix);
    }
    else
    {
        // Apply to all bones
        for (DWORD i = 0; i < lpPhy->lpMotion->dwBoneCount; i++)
        {
            lpPhy->lpMotion->matrix[i] = XMMatrixMultiply(lpPhy->lpMotion->matrix[i], *matrix);
        }
    }
}

void Phy_SetColor(CHPhy* lpPhy, float alpha, float red, float green, float blue)
{
    if (!lpPhy)
        return;
    
    lpPhy->fA = alpha;
    lpPhy->fR = red;
    lpPhy->fG = green;
    lpPhy->fB = blue;
}

void Phy_ClearMatrix(CHPhy* lpPhy)
{
    if (!lpPhy || !lpPhy->lpMotion)
        return;
    
    for (DWORD i = 0; i < lpPhy->lpMotion->dwBoneCount; i++)
    {
        lpPhy->lpMotion->matrix[i] = XMMatrixIdentity();
    }
}

void Phy_ChangeTexture(CHPhy* lpPhy, int nTexID, int nTexID2)
{
    if (!lpPhy)
        return;
    
    lpPhy->nTex = nTexID;
    lpPhy->nTex2 = nTexID2;
}

// Internal implementation placeholders
namespace CHPhyInternal {

void ProcessVertexBlending(CHPhy* phy)
{
    // Placeholder for vertex blending implementation
}

void InterpolateKeyframes(CHMotion* motion, float frame, XMMATRIX* outMatrices)
{
    // Placeholder for keyframe interpolation
}

void ApplyBoneTransforms(CHPhy* phy)
{
    // Placeholder for bone transformation
}

void ApplyMorphTargets(CHPhy* phy)
{
    // Placeholder for morph target processing
}

BOOL CreateVertexBuffers(CHPhy* phy)
{
    // Placeholder for DX11 vertex buffer creation
    return TRUE;
}

BOOL CreateIndexBuffers(CHPhy* phy)
{
    // Placeholder for DX11 index buffer creation
    return TRUE;
}

BOOL CreateBoneMatrixBuffer(CHPhy* phy)
{
    // Placeholder for bone matrix constant buffer creation
    return TRUE;
}

void UpdateVertexBuffer(CHPhy* phy, bool isAlpha)
{
    // Placeholder for vertex buffer updates
}

void UpdateBoneMatrices(CHPhy* phy)
{
    // Placeholder for bone matrix updates
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
    // Placeholder for normal mesh rendering
    return TRUE;
}

BOOL RenderAlphaMesh(CHPhy* phy, bool enableZ, int srcBlend, int destBlend)
{
    // Placeholder for alpha mesh rendering
    return TRUE;
}

void SetupPhyRenderStates()
{
    // Placeholder for physics render state setup
}

BOOL LoadMotionFromFile(FILE* file, CHMotion** motion)
{
    // Placeholder for motion loading from file
    return FALSE;
}

BOOL LoadMotionFromPack(HANDLE handle, CHMotion** motion)
{
    // Placeholder for motion loading from packed file
    return FALSE;
}

void ProcessMotionKeyframes(CHMotion* motion)
{
    // Placeholder for motion keyframe processing
}

BOOL LoadPhyFromFile(FILE* file, CHPhy** phy, bool loadTextures)
{
    // Placeholder for physics object loading from file
    return FALSE;
}

BOOL LoadPhyFromPack(HANDLE handle, CHPhy** phy, bool loadTextures)
{
    // Placeholder for physics object loading from packed file
    return FALSE;
}

BOOL SavePhyToFile(const char* filename, CHPhy* phy, bool newFile)
{
    // Placeholder for physics object saving to file
    return FALSE;
}

} // namespace CHPhyInternal