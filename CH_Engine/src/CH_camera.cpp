#include "CH_camera.h"
#include "CH_main.h"

// External version string from CH_main.h
extern const char CH_VERSION[64];

CH_CORE_DLL_API
void Camera_Clear(CHCamera* lpCamera)
{
    lpCamera->lpName = nullptr;
    lpCamera->lpFrom = nullptr;
    lpCamera->lpTo = nullptr;
    lpCamera->fNear = 10.0f;
    lpCamera->fFar = 10000.0f;
    lpCamera->fFov = CHCameraMath::ToRadian(45.0f); // D3DXToRadian(45)

    lpCamera->dwFrameCount = 0;
    lpCamera->nFrame = 0;
}

CH_CORE_DLL_API
BOOL Camera_Load(CHCamera** lpCamera,
                const char* lpName,
                DWORD dwIndex)
{
    FILE* file = fopen(lpName, "rb");
    if (!file)
        return FALSE;

    char version[64];
    if (fread(version, sizeof(char), 16, file) != 16)
    {
        fclose(file);
        return FALSE;
    }
    version[16] = '\0';
    
    if (strcmp(version, CH_VERSION) != 0)
    {
        ErrorMessage("Camera version error");
        fclose(file);
        return FALSE;
    }

    ChunkHeader chunk;
    DWORD add = 0;

    while (1)
    {
        if (fread(&chunk, sizeof(ChunkHeader), 1, file) != 1)
        {
            fclose(file);
            return FALSE;
        }
        
        if (feof(file))
        {
            fclose(file);
            return FALSE;
        }
        
        if (chunk.byChunkID[0] == 'C' &&
            chunk.byChunkID[1] == 'A' &&
            chunk.byChunkID[2] == 'M' &&
            chunk.byChunkID[3] == 'E')
        {
            if (add < dwIndex)
            {
                add++;
                fseek(file, chunk.dwChunkSize, SEEK_CUR);
                continue;
            }

            *lpCamera = new CHCamera;
            Camera_Clear(*lpCamera);

            // Load camera name
            DWORD temp;
            fread(&temp, sizeof(DWORD), 1, file);
            (*lpCamera)->lpName = new char[temp + 1];
            fread((*lpCamera)->lpName, 1, temp, file);
            (*lpCamera)->lpName[temp] = '\0';
            
            // Skip FOV field (as in original - it's commented out)
            fseek(file, sizeof(float), SEEK_CUR);
            
            // Load frame count
            fread(&(*lpCamera)->dwFrameCount, sizeof(DWORD), 1, file);
            
            // Load position arrays (convert from D3DXVECTOR3 to XMVECTOR)
            (*lpCamera)->lpFrom = new XMVECTOR[(*lpCamera)->dwFrameCount];
            (*lpCamera)->lpTo = new XMVECTOR[(*lpCamera)->dwFrameCount];
            
            // Read position data
            for (DWORD i = 0; i < (*lpCamera)->dwFrameCount; i++)
            {
                float x, y, z;
                fread(&x, sizeof(float), 1, file);
                fread(&y, sizeof(float), 1, file);
                fread(&z, sizeof(float), 1, file);
                (*lpCamera)->lpFrom[i] = CHCameraMath::VectorFromD3DX(x, y, z);
            }
            
            // Read target data
            for (DWORD i = 0; i < (*lpCamera)->dwFrameCount; i++)
            {
                float x, y, z;
                fread(&x, sizeof(float), 1, file);
                fread(&y, sizeof(float), 1, file);
                fread(&z, sizeof(float), 1, file);
                (*lpCamera)->lpTo[i] = CHCameraMath::VectorFromD3DX(x, y, z);
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
BOOL Camera_Save(char* lpName, CHCamera* lpCamera, BOOL bNew)
{
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);

    // Camera chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'C';
    chunk.byChunkID[1] = 'A';
    chunk.byChunkID[2] = 'M';
    chunk.byChunkID[3] = 'E';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);

    // Name
    DWORD length = static_cast<DWORD>(strlen(lpCamera->lpName));
    fwrite(&length, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);

    fwrite(lpCamera->lpName, sizeof(char), length, file);
    chunk.dwChunkSize += sizeof(char) * length;

    // FOV
    fwrite(&lpCamera->fFov, sizeof(float), 1, file);
    chunk.dwChunkSize += sizeof(float);

    // Frame count
    fwrite(&lpCamera->dwFrameCount, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);

    // Position data (convert XMVECTOR back to D3DXVECTOR3 format)
    for (DWORD i = 0; i < lpCamera->dwFrameCount; i++)
    {
        float x, y, z;
        CHCameraMath::VectorToD3DX(lpCamera->lpFrom[i], x, y, z);
        fwrite(&x, sizeof(float), 1, file);
        fwrite(&y, sizeof(float), 1, file);
        fwrite(&z, sizeof(float), 1, file);
    }
    chunk.dwChunkSize += sizeof(float) * 3 * lpCamera->dwFrameCount;

    // Target data
    for (DWORD i = 0; i < lpCamera->dwFrameCount; i++)
    {
        float x, y, z;
        CHCameraMath::VectorToD3DX(lpCamera->lpTo[i], x, y, z);
        fwrite(&x, sizeof(float), 1, file);
        fwrite(&y, sizeof(float), 1, file);
        fwrite(&z, sizeof(float), 1, file);
    }
    chunk.dwChunkSize += sizeof(float) * 3 * lpCamera->dwFrameCount;

    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);

    fclose(file);
    return TRUE;
}

CH_CORE_DLL_API
void Camera_Unload(CHCamera** lpCamera)
{
    if (!lpCamera || !*lpCamera)
        return;
        
    delete[] (*lpCamera)->lpName;
    delete[] (*lpCamera)->lpFrom;
    delete[] (*lpCamera)->lpTo;
    delete *lpCamera;
    *lpCamera = nullptr;
}

CH_CORE_DLL_API
void Camera_NextFrame(CHCamera* lpCamera, int nStep)
{
    if (lpCamera->dwFrameCount > 0)
    {
        lpCamera->nFrame = (lpCamera->nFrame + nStep) % lpCamera->dwFrameCount;
    }
}

CH_CORE_DLL_API
BOOL Camera_BuildView(CHCamera* lpCamera, BOOL bSet)
{
    if (!lpCamera || !lpCamera->lpFrom || !lpCamera->lpTo || lpCamera->dwFrameCount == 0)
        return FALSE;

    XMVECTOR up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    g_ViewMatrix = CHCameraInternal::CreateLookAtMatrix(
        lpCamera->lpFrom[lpCamera->nFrame],
        lpCamera->lpTo[lpCamera->nFrame],
        up);

    if (bSet)
    {
        // Update shader constant buffer with new view matrix
        CHInternal::g_CompatibilityShaderManager.UpdateConstantBuffer(XMMatrixIdentity(), g_ViewMatrix, g_ProjectMatrix);
    }
    
    return TRUE;
}

CH_CORE_DLL_API
BOOL Camera_BuildProject(CHCamera* lpCamera, BOOL bSet)
{
    if (!lpCamera)
        return FALSE;

    float aspectRatio = static_cast<float>(g_DisplayMode.Width) / static_cast<float>(g_DisplayMode.Height);
    g_ProjectMatrix = CHCameraInternal::CreatePerspectiveMatrix(
        lpCamera->fFov,
        aspectRatio,
        lpCamera->fNear,
        lpCamera->fFar);

    if (bSet)
    {
        // Update shader constant buffer with new projection matrix
        CHInternal::g_CompatibilityShaderManager.UpdateConstantBuffer(XMMatrixIdentity(), g_ViewMatrix, g_ProjectMatrix);
    }
    
    return TRUE;
}

CH_CORE_DLL_API
BOOL Camera_BuildOrtho(CHCamera* lpCamera,
                      float fWidth,
                      float fHeight,
                      BOOL bSet)
{
    if (!lpCamera)
        return FALSE;

    g_ProjectMatrix = CHCameraInternal::CreateOrthographicMatrix(
        fWidth,
        fHeight,
        lpCamera->fNear,
        lpCamera->fFar);

    if (bSet)
    {
        // Update shader constant buffer with new projection matrix
        CHInternal::g_CompatibilityShaderManager.UpdateConstantBuffer(XMMatrixIdentity(), g_ViewMatrix, g_ProjectMatrix);
    }
    
    return TRUE;
}

CH_CORE_DLL_API
void Camera_Process1stRotate(CHCamera* lpCamera,
                           float fHRadian,
                           float fVRadian)
{
    if (!lpCamera || !lpCamera->lpFrom || !lpCamera->lpTo || lpCamera->dwFrameCount == 0)
        return;

    XMVECTOR vec = XMVectorSubtract(lpCamera->lpTo[lpCamera->nFrame], lpCamera->lpFrom[lpCamera->nFrame]);

    // Horizontal rotation (around Z axis)
    XMMATRIX rotationH = XMMatrixRotationZ(fHRadian);
    vec = XMVector3TransformCoord(vec, rotationH);

    // Vertical rotation
    XMVECTOR axis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR rotAxis = XMVector3Cross(axis, vec);
    
    XMMATRIX rotationV = XMMatrixRotationAxis(rotAxis, fVRadian);
    XMVECTOR vecEnd = XMVector3TransformCoord(vec, rotationV);

    // Angle limitation
    axis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR limit = XMVector3Normalize(vecEnd);
    
    XMVECTOR dotProduct = XMVector3Dot(axis, limit);
    float radian = acosf(XMVectorGetX(dotProduct));

    float minAngle = CHCameraMath::ToRadian(10.0f);
    float maxAngle = CHCameraMath::ToRadian(170.0f);
    
    if (radian > minAngle && radian < maxAngle)
    {
        lpCamera->lpTo[lpCamera->nFrame] = XMVectorAdd(lpCamera->lpFrom[lpCamera->nFrame], vecEnd);
    }
    else
    {
        lpCamera->lpTo[lpCamera->nFrame] = XMVectorAdd(lpCamera->lpFrom[lpCamera->nFrame], vec);
    }
}

CH_CORE_DLL_API
void Camera_Process1stTranslate(CHCamera* lpCamera,
                              float fDirRadian,
                              float fStep)
{
    if (!lpCamera || !lpCamera->lpFrom || !lpCamera->lpTo || lpCamera->dwFrameCount == 0)
        return;

    XMVECTOR axis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR vec = XMVectorSubtract(lpCamera->lpTo[lpCamera->nFrame], lpCamera->lpFrom[lpCamera->nFrame]);
    
    // Calculate movement plane
    XMVECTOR moveAxis = XMVector3Cross(vec, axis);
    moveAxis = XMVector3Cross(vec, moveAxis);
    moveAxis = XMVector3Normalize(moveAxis);

    XMMATRIX rotation = XMMatrixRotationAxis(moveAxis, fDirRadian);
    vec = XMVector3TransformCoord(vec, rotation);

    XMVECTOR zero = XMVectorZero();
    float vecLength = XMVectorGetX(XMVector3Length(vec));
    
    if (vecLength > 0.0f)
    {
        vec = XMVectorLerp(zero, vec, fStep / vecLength);
        
        lpCamera->lpFrom[lpCamera->nFrame] = XMVectorAdd(lpCamera->lpFrom[lpCamera->nFrame], vec);
        lpCamera->lpTo[lpCamera->nFrame] = XMVectorAdd(lpCamera->lpTo[lpCamera->nFrame], vec);
    }
}

CH_CORE_DLL_API
void Camera_ProcessXYTranslate(CHCamera* lpCamera,
                             float fDirRadian,
                             float fStep)
{
    if (!lpCamera || !lpCamera->lpFrom || !lpCamera->lpTo || lpCamera->dwFrameCount == 0)
        return;

    XMVECTOR axis = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    XMVECTOR vec = XMVectorSubtract(lpCamera->lpTo[lpCamera->nFrame], lpCamera->lpFrom[lpCamera->nFrame]);
    
    // Project to XY plane (set Z to 0)
    vec = XMVectorSetZ(vec, 0.0f);
    
    XMMATRIX rotation = XMMatrixRotationAxis(axis, fDirRadian);
    vec = XMVector3TransformCoord(vec, rotation);

    XMVECTOR zero = XMVectorZero();
    float vecLength = XMVectorGetX(XMVector3Length(vec));
    
    if (vecLength > 0.0f)
    {
        vec = XMVectorLerp(zero, vec, fStep / vecLength);
        
        lpCamera->lpFrom[lpCamera->nFrame] = XMVectorAdd(lpCamera->lpFrom[lpCamera->nFrame], vec);
        lpCamera->lpTo[lpCamera->nFrame] = XMVectorAdd(lpCamera->lpTo[lpCamera->nFrame], vec);
    }
}

// Internal implementation
namespace CHCameraInternal {

XMMATRIX CreateLookAtMatrix(const XMVECTOR& from, const XMVECTOR& to, const XMVECTOR& up)
{
    return XMMatrixLookAtLH(from, to, up);
}

XMMATRIX CreatePerspectiveMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    return XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
}

XMMATRIX CreateOrthographicMatrix(float width, float height, float nearPlane, float farPlane)
{
    return XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
}

void RotateVector(XMVECTOR& vector, const XMVECTOR& axis, float radians)
{
    XMMATRIX rotation = XMMatrixRotationAxis(axis, radians);
    vector = XMVector3TransformCoord(vector, rotation);
}

void ClampVerticalRotation(XMVECTOR& target, const XMVECTOR& from, float minAngle, float maxAngle)
{
    XMVECTOR up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR direction = XMVector3Normalize(XMVectorSubtract(target, from));
    
    XMVECTOR dotProduct = XMVector3Dot(up, direction);
    float angle = acosf(XMVectorGetX(dotProduct));
    
    if (angle < minAngle || angle > maxAngle)
    {
        // Clamp the angle
        float clampedAngle = (angle < minAngle) ? minAngle : maxAngle;
        
        // Recalculate direction with clamped angle
        XMVECTOR side = XMVector3Cross(up, direction);
        side = XMVector3Normalize(side);
        
        XMMATRIX rotation = XMMatrixRotationAxis(side, clampedAngle - XM_PIDIV2);
        XMVECTOR newDirection = XMVector3TransformCoord(up, rotation);
        
        float distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(target, from)));
        target = XMVectorAdd(from, XMVectorScale(newDirection, distance));
    }
}

BOOL ReadCameraChunk(FILE* file, CHCamera* camera)
{
    // This is a placeholder for more complex chunk reading if needed
    return TRUE;
}

BOOL WriteCameraChunk(FILE* file, CHCamera* camera, ChunkHeader& chunk)
{
    // This is a placeholder for more complex chunk writing if needed
    return TRUE;
}

} // namespace CHCameraInternal