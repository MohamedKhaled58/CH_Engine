#ifndef _CH_omni_h_
#define _CH_omni_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"

// Omnidirectional light structure (maintains exact same layout as C3Omni)
struct CHOmni {
    char* lpName;               // Light name
    XMVECTOR pos;              // Light position
    XMFLOAT4 color;            // Light color (RGBA)
    float fRadius;              // Light radius
    float fAttenuation;         // Attenuation factor
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Omni_Clear(CHOmni* lpOmni);

CH_CORE_DLL_API
BOOL Omni_Load(CHOmni** lpOmni,
              char* lpName,
              DWORD dwIndex);

CH_CORE_DLL_API
BOOL Omni_Save(char* lpName, CHOmni* lpOmni, BOOL bNew);

CH_CORE_DLL_API
void Omni_Unload(CHOmni** lpOmni);

// Internal lighting implementation
namespace CHOmniInternal {
    // Light calculations
    XMFLOAT4 CalculateLightContribution(CHOmni* light, XMVECTOR worldPos, XMVECTOR normal);
    float CalculateAttenuation(CHOmni* light, XMVECTOR worldPos);
    
    // Light management
    void SetupLightingStates();
    void ApplyOmnidirectionalLight(CHOmni* light, UINT lightIndex);
    
    // File I/O
    BOOL ReadOmniChunk(FILE* file, CHOmni* omni);
    BOOL WriteOmniChunk(FILE* file, CHOmni* omni, ChunkHeader& chunk);
}

// Compatibility types
typedef CHOmni C3Omni;

#endif // _CH_omni_h_