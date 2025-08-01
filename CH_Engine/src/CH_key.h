#ifndef _CH_key_h_
#define _CH_key_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"

// Remove redundant windows.h include (already in CH_common.h)

// Animation frame structure
struct CHFrame {
    int nFrame;                 // Keyframe position
    float fParam[1];           // Float parameter
    BOOL bParam[1];            // Boolean parameter
    int nParam[1];             // Integer parameter
};

// Animation key structure (maintains exact same layout as C3Key)
struct CHKey {
    DWORD dwAlphas;            // Number of alpha keyframes
    CHFrame* lpAlphas;         // Alpha keyframes

    DWORD dwDraws;             // Number of draw keyframes
    CHFrame* lpDraws;          // Draw keyframes

    DWORD dwChangeTexs;        // Number of texture change keyframes
    CHFrame* lpChangeTexs;     // Texture change keyframes
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Key_Clear(CHKey* lpKey);

CH_CORE_DLL_API
BOOL Key_Load(CHKey** lpKey, char* lpName, DWORD dwIndex);

CH_CORE_DLL_API
BOOL Key_Save(char* lpName, CHKey* lpKey, BOOL bNew);

CH_CORE_DLL_API
void Key_Unload(CHKey** lpKey);

CH_CORE_DLL_API
BOOL Key_ProcessAlpha(CHKey* lpKey, DWORD dwFrame, DWORD dwFrames, float* fReturn);

CH_CORE_DLL_API
BOOL Key_ProcessDraw(CHKey* lpKey, DWORD dwFrame, BOOL* bReturn);

CH_CORE_DLL_API
BOOL Key_ProcessChangeTex(CHKey* lpKey, DWORD dwFrame, int* nReturn);

// Internal helper functions
namespace CHKeyInternal {
    // Keyframe interpolation utilities
    float InterpolateFloat(CHFrame* frames, DWORD frameCount, DWORD currentFrame, DWORD totalFrames);
    BOOL InterpolateBool(CHFrame* frames, DWORD frameCount, DWORD currentFrame);
    int InterpolateInt(CHFrame* frames, DWORD frameCount, DWORD currentFrame);
    
    // File I/O utilities
    BOOL LoadKeyFromFile(FILE* file, CHKey** key, DWORD index);
    BOOL SaveKeyToFile(const char* filename, CHKey* key, BOOL newFile);
    
    // Frame processing
    CHFrame* FindKeyframe(CHFrame* frames, DWORD frameCount, DWORD targetFrame);
    CHFrame* FindPreviousKeyframe(CHFrame* frames, DWORD frameCount, DWORD targetFrame);
    CHFrame* FindNextKeyframe(CHFrame* frames, DWORD frameCount, DWORD targetFrame);
}

// Compatibility types
typedef CHKey C3Key;
typedef CHFrame C3Frame;

#endif // _CH_key_h_