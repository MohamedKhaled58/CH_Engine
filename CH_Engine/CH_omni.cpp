#include "CH_omni.h"

extern const char CH_VERSION[64];

void Omni_Clear(CHOmni* lpOmni)
{
    if (!lpOmni)
        return;
    
    delete[] lpOmni->lpName;
    lpOmni->lpName = nullptr;
    lpOmni->pos = XMVectorZero();
    lpOmni->color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    lpOmni->fRadius = 100.0f;
    lpOmni->fAttenuation = 1.0f;
}

BOOL Omni_Load(CHOmni** lpOmni, char* lpName, DWORD dwIndex)
{
    if (!lpOmni || !lpName)
        return FALSE;
    
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
        ErrorMessage("Omni version error");
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
        
        if (chunk.byChunkID[0] == 'O' &&
            chunk.byChunkID[1] == 'M' &&
            chunk.byChunkID[2] == 'N' &&
            chunk.byChunkID[3] == 'I')
        {
            if (add < dwIndex)
            {
                add++;
                fseek(file, chunk.dwChunkSize, SEEK_CUR);
                continue;
            }
            
            *lpOmni = new CHOmni;
            Omni_Clear(*lpOmni);
            
            // Load light name
            DWORD nameLen;
            fread(&nameLen, sizeof(DWORD), 1, file);
            if (nameLen > 0)
            {
                (*lpOmni)->lpName = new char[nameLen + 1];
                fread((*lpOmni)->lpName, sizeof(char), nameLen, file);
                (*lpOmni)->lpName[nameLen] = '\0';
            }
            
            // Load position (convert from D3DXVECTOR3 to XMVECTOR)
            float x, y, z;
            fread(&x, sizeof(float), 1, file);
            fread(&y, sizeof(float), 1, file);
            fread(&z, sizeof(float), 1, file);
            (*lpOmni)->pos = XMVectorSet(x, y, z, 1.0f);
            
            // Load color (convert from D3DXCOLOR to XMFLOAT4)
            fread(&(*lpOmni)->color, sizeof(XMFLOAT4), 1, file);
            
            // Load radius and attenuation
            fread(&(*lpOmni)->fRadius, sizeof(float), 1, file);
            fread(&(*lpOmni)->fAttenuation, sizeof(float), 1, file);
            
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

BOOL Omni_Save(char* lpName, CHOmni* lpOmni, BOOL bNew)
{
    if (!lpName || !lpOmni)
        return FALSE;
    
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);
    
    // Omni light chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'O';
    chunk.byChunkID[1] = 'M';
    chunk.byChunkID[2] = 'N';
    chunk.byChunkID[3] = 'I';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);
    
    // Name
    DWORD nameLen = lpOmni->lpName ? static_cast<DWORD>(strlen(lpOmni->lpName)) : 0;
    fwrite(&nameLen, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    
    if (nameLen > 0)
    {
        fwrite(lpOmni->lpName, sizeof(char), nameLen, file);
        chunk.dwChunkSize += sizeof(char) * nameLen;
    }
    
    // Position (convert XMVECTOR to D3DXVECTOR3)
    XMFLOAT3 pos;
    XMStoreFloat3(&pos, lpOmni->pos);
    fwrite(&pos, sizeof(XMFLOAT3), 1, file);
    chunk.dwChunkSize += sizeof(XMFLOAT3);
    
    // Color
    fwrite(&lpOmni->color, sizeof(XMFLOAT4), 1, file);
    chunk.dwChunkSize += sizeof(XMFLOAT4);
    
    // Radius and attenuation
    fwrite(&lpOmni->fRadius, sizeof(float), 1, file);
    fwrite(&lpOmni->fAttenuation, sizeof(float), 1, file);
    chunk.dwChunkSize += sizeof(float) * 2;
    
    // Update chunk size
    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);
    
    fclose(file);
    return TRUE;
}

void Omni_Unload(CHOmni** lpOmni)
{
    if (!lpOmni || !*lpOmni)
        return;
    
    Omni_Clear(*lpOmni);
    delete *lpOmni;
    *lpOmni = nullptr;
}

// Internal implementation
namespace CHOmniInternal {

XMFLOAT4 CalculateLightContribution(CHOmni* light, XMVECTOR worldPos, XMVECTOR normal)
{
    if (!light)
        return XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    
    // Calculate light direction
    XMVECTOR lightDir = XMVectorSubtract(light->pos, worldPos);
    float distance = XMVectorGetX(XMVector3Length(lightDir));
    lightDir = XMVector3Normalize(lightDir);
    
    // Calculate attenuation
    float attenuation = CalculateAttenuation(light, worldPos);
    
    // Calculate diffuse lighting (Lambertian)
    float ndotl = XMVectorGetX(XMVector3Dot(normal, lightDir));
    ndotl = max(0.0f, ndotl);
    
    // Combine light color with attenuation and diffuse factor
    XMFLOAT4 result;
    result.x = light->color.x * ndotl * attenuation;
    result.y = light->color.y * ndotl * attenuation;
    result.z = light->color.z * ndotl * attenuation;
    result.w = light->color.w;
    
    return result;
}

float CalculateAttenuation(CHOmni* light, XMVECTOR worldPos)
{
    if (!light)
        return 0.0f;
    
    // Calculate distance from light to position
    XMVECTOR lightToPos = XMVectorSubtract(worldPos, light->pos);
    float distance = XMVectorGetX(XMVector3Length(lightToPos));
    
    // Prevent division by zero
    if (distance < 0.001f)
        return 1.0f;
    
    // Check if within light radius
    if (distance > light->fRadius)
        return 0.0f;
    
    // Simple linear attenuation: 1 - (distance / radius) * attenuation
    float normalizedDistance = distance / light->fRadius;
    float attenuation = 1.0f - (normalizedDistance * light->fAttenuation);
    
    return max(0.0f, attenuation);
}

void SetupLightingStates()
{
    // Set render states for lighting
    SetRenderState(CH_RS_LIGHTING, TRUE);
    SetRenderState(CH_RS_AMBIENT, 0x404040); // Dark gray ambient
    
    // Enable diffuse lighting on texture stage 0
    SetTextureStageState(0, CH_TSS_COLOROP, 4); // D3DTOP_MODULATE equivalent
    SetTextureStageState(0, CH_TSS_COLORARG1, 2); // D3DTA_TEXTURE equivalent
    SetTextureStageState(0, CH_TSS_COLORARG2, 0); // D3DTA_DIFFUSE equivalent
}

void ApplyOmnidirectionalLight(CHOmni* light, UINT lightIndex)
{
    if (!light || lightIndex >= 8) // DirectX typically supports up to 8 lights
        return;
    
    // In DirectX 11, lighting is handled in shaders rather than fixed-function pipeline
    // This function would update a light constant buffer for shader use
    
    // TODO: Update light constant buffer with light parameters
    // - Position: light->pos
    // - Color: light->color
    // - Radius: light->fRadius
    // - Attenuation: light->fAttenuation
}

BOOL ReadOmniChunk(FILE* file, CHOmni* omni)
{
    if (!file || !omni)
        return FALSE;
    
    // This would be used for more complex chunk reading if needed
    // For now, the main loading logic is in Omni_Load
    return TRUE;
}

BOOL WriteOmniChunk(FILE* file, CHOmni* omni, ChunkHeader& chunk)
{
    if (!file || !omni)
        return FALSE;
    
    // This would be used for more complex chunk writing if needed
    // For now, the main saving logic is in Omni_Save
    return TRUE;
}

} // namespace CHOmniInternal