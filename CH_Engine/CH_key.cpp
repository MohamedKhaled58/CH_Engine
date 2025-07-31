#include "CH_key.h"

CH_CORE_DLL_API
void Key_Clear(CHKey* lpKey)
{
    if (!lpKey)
        return;
        
    // Clear alpha keyframes
    if (lpKey->lpAlphas)
    {
        delete[] lpKey->lpAlphas;
        lpKey->lpAlphas = nullptr;
    }
    lpKey->dwAlphas = 0;
    
    // Clear draw keyframes
    if (lpKey->lpDraws)
    {
        delete[] lpKey->lpDraws;
        lpKey->lpDraws = nullptr;
    }
    lpKey->dwDraws = 0;
    
    // Clear texture change keyframes
    if (lpKey->lpChangeTexs)
    {
        delete[] lpKey->lpChangeTexs;
        lpKey->lpChangeTexs = nullptr;
    }
    lpKey->dwChangeTexs = 0;
}

CH_CORE_DLL_API
BOOL Key_Load(CHKey** lpKey, char* lpName, DWORD dwIndex)
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
        ErrorMessage("Key version error");
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
        
        if (chunk.byChunkID[0] == 'K' &&
            chunk.byChunkID[1] == 'E' &&
            chunk.byChunkID[2] == 'Y' &&
            chunk.byChunkID[3] == 'S')
        {
            if (add < dwIndex)
            {
                add++;
                fseek(file, chunk.dwChunkSize, SEEK_CUR);
                continue;
            }

            *lpKey = new CHKey;
            Key_Clear(*lpKey);

            // Load alpha keyframes
            fread(&(*lpKey)->dwAlphas, sizeof(DWORD), 1, file);
            if ((*lpKey)->dwAlphas > 0)
            {
                (*lpKey)->lpAlphas = new CHFrame[(*lpKey)->dwAlphas];
                fread((*lpKey)->lpAlphas, sizeof(CHFrame), (*lpKey)->dwAlphas, file);
            }
            
            // Load draw keyframes
            fread(&(*lpKey)->dwDraws, sizeof(DWORD), 1, file);
            if ((*lpKey)->dwDraws > 0)
            {
                (*lpKey)->lpDraws = new CHFrame[(*lpKey)->dwDraws];
                fread((*lpKey)->lpDraws, sizeof(CHFrame), (*lpKey)->dwDraws, file);
            }
            
            // Load texture change keyframes
            fread(&(*lpKey)->dwChangeTexs, sizeof(DWORD), 1, file);
            if ((*lpKey)->dwChangeTexs > 0)
            {
                (*lpKey)->lpChangeTexs = new CHFrame[(*lpKey)->dwChangeTexs];
                fread((*lpKey)->lpChangeTexs, sizeof(CHFrame), (*lpKey)->dwChangeTexs, file);
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
BOOL Key_Save(char* lpName, CHKey* lpKey, BOOL bNew)
{
    FILE* file = fopen(lpName, bNew ? "w+b" : "r+b");
    if (!file)
        return FALSE;
    fseek(file, 0, SEEK_END);

    // Key chunk
    ChunkHeader chunk;
    chunk.byChunkID[0] = 'K';
    chunk.byChunkID[1] = 'E';
    chunk.byChunkID[2] = 'Y';
    chunk.byChunkID[3] = 'S';
    chunk.dwChunkSize = 0;
    fwrite(&chunk, sizeof(chunk), 1, file);

    // Alpha keyframes
    fwrite(&lpKey->dwAlphas, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    if (lpKey->dwAlphas > 0)
    {
        fwrite(lpKey->lpAlphas, sizeof(CHFrame), lpKey->dwAlphas, file);
        chunk.dwChunkSize += sizeof(CHFrame) * lpKey->dwAlphas;
    }
    
    // Draw keyframes
    fwrite(&lpKey->dwDraws, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    if (lpKey->dwDraws > 0)
    {
        fwrite(lpKey->lpDraws, sizeof(CHFrame), lpKey->dwDraws, file);
        chunk.dwChunkSize += sizeof(CHFrame) * lpKey->dwDraws;
    }
    
    // Texture change keyframes
    fwrite(&lpKey->dwChangeTexs, sizeof(DWORD), 1, file);
    chunk.dwChunkSize += sizeof(DWORD);
    if (lpKey->dwChangeTexs > 0)
    {
        fwrite(lpKey->lpChangeTexs, sizeof(CHFrame), lpKey->dwChangeTexs, file);
        chunk.dwChunkSize += sizeof(CHFrame) * lpKey->dwChangeTexs;
    }

    fseek(file, -static_cast<int>(chunk.dwChunkSize + sizeof(chunk)), SEEK_CUR);
    fwrite(&chunk, sizeof(chunk), 1, file);
    fseek(file, 0, SEEK_END);

    fclose(file);
    return TRUE;
}

CH_CORE_DLL_API
void Key_Unload(CHKey** lpKey)
{
    if (!lpKey || !*lpKey)
        return;
        
    Key_Clear(*lpKey);
    delete *lpKey;
    *lpKey = nullptr;
}

CH_CORE_DLL_API
BOOL Key_ProcessAlpha(CHKey* lpKey, DWORD dwFrame, DWORD dwFrames, float* fReturn)
{
    if (!lpKey || !fReturn || lpKey->dwAlphas == 0)
        return FALSE;

    *fReturn = CHKeyInternal::InterpolateFloat(lpKey->lpAlphas, lpKey->dwAlphas, dwFrame, dwFrames);
    return TRUE;
}

CH_CORE_DLL_API
BOOL Key_ProcessDraw(CHKey* lpKey, DWORD dwFrame, BOOL* bReturn)
{
    if (!lpKey || !bReturn || lpKey->dwDraws == 0)
        return FALSE;

    *bReturn = CHKeyInternal::InterpolateBool(lpKey->lpDraws, lpKey->dwDraws, dwFrame);
    return TRUE;
}

CH_CORE_DLL_API
BOOL Key_ProcessChangeTex(CHKey* lpKey, DWORD dwFrame, int* nReturn)
{
    if (!lpKey || !nReturn || lpKey->dwChangeTexs == 0)
        return FALSE;

    *nReturn = CHKeyInternal::InterpolateInt(lpKey->lpChangeTexs, lpKey->dwChangeTexs, dwFrame);
    return TRUE;
}

// Internal implementation
namespace CHKeyInternal {

float InterpolateFloat(CHFrame* frames, DWORD frameCount, DWORD currentFrame, DWORD totalFrames)
{
    if (!frames || frameCount == 0)
        return 0.0f;

    // Find the keyframe at or before the current frame
    CHFrame* currentKeyframe = nullptr;
    CHFrame* nextKeyframe = nullptr;
    
    for (DWORD i = 0; i < frameCount; i++)
    {
        if (frames[i].nFrame <= static_cast<int>(currentFrame))
        {
            currentKeyframe = &frames[i];
            if (i + 1 < frameCount)
                nextKeyframe = &frames[i + 1];
        }
        else
        {
            break;
        }
    }
    
    if (!currentKeyframe)
        return frames[0].fParam[0];
    
    if (!nextKeyframe || currentKeyframe->nFrame == static_cast<int>(currentFrame))
        return currentKeyframe->fParam[0];
    
    // Linear interpolation between keyframes
    float t = static_cast<float>(currentFrame - currentKeyframe->nFrame) / 
              static_cast<float>(nextKeyframe->nFrame - currentKeyframe->nFrame);
    
    return currentKeyframe->fParam[0] + t * (nextKeyframe->fParam[0] - currentKeyframe->fParam[0]);
}

BOOL InterpolateBool(CHFrame* frames, DWORD frameCount, DWORD currentFrame)
{
    if (!frames || frameCount == 0)
        return FALSE;

    for (DWORD i = 0; i < frameCount; i++)
    {
        if (frames[i].nFrame <= static_cast<int>(currentFrame))
        {
            if (i + 1 < frameCount && frames[i + 1].nFrame > static_cast<int>(currentFrame))
            {
                return frames[i].bParam[0];
            }
            else if (i == frameCount - 1)
            {
                return frames[i].bParam[0];
            }
        }
    }
    
    return frames[0].bParam[0];
}

int InterpolateInt(CHFrame* frames, DWORD frameCount, DWORD currentFrame)
{
    if (!frames || frameCount == 0)
        return 0;

    for (DWORD i = 0; i < frameCount; i++)
    {
        if (frames[i].nFrame <= static_cast<int>(currentFrame))
        {
            if (i + 1 < frameCount && frames[i + 1].nFrame > static_cast<int>(currentFrame))
            {
                return frames[i].nParam[0];
            }
            else if (i == frameCount - 1)
            {
                return frames[i].nParam[0];
            }
        }
    }
    
    return frames[0].nParam[0];
}

CHFrame* FindKeyframe(CHFrame* frames, DWORD frameCount, DWORD targetFrame)
{
    for (DWORD i = 0; i < frameCount; i++)
    {
        if (frames[i].nFrame == static_cast<int>(targetFrame))
            return &frames[i];
    }
    return nullptr;
}

CHFrame* FindPreviousKeyframe(CHFrame* frames, DWORD frameCount, DWORD targetFrame)
{
    CHFrame* prev = nullptr;
    for (DWORD i = 0; i < frameCount; i++)
    {
        if (frames[i].nFrame < static_cast<int>(targetFrame))
            prev = &frames[i];
        else
            break;
    }
    return prev;
}

CHFrame* FindNextKeyframe(CHFrame* frames, DWORD frameCount, DWORD targetFrame)
{
    for (DWORD i = 0; i < frameCount; i++)
    {
        if (frames[i].nFrame > static_cast<int>(targetFrame))
            return &frames[i];
    }
    return nullptr;
}

BOOL LoadKeyFromFile(FILE* file, CHKey** key, DWORD index)
{
    // Implementation would be similar to Key_Load but more focused
    return FALSE; // Placeholder
}

BOOL SaveKeyToFile(const char* filename, CHKey* key, BOOL newFile)
{
    // Implementation would be similar to Key_Save
    return FALSE; // Placeholder
}

} // namespace CHKeyInternal