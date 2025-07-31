#ifndef _CH_camera_h_
#define _CH_camera_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"

// Camera structure (maintaining exact same layout as C3Camera)
struct CHCamera {
    char* lpName;           // Camera name
    XMVECTOR* lpFrom;       // Position array (for animation)
    XMVECTOR* lpTo;         // Target array (for animation)
    float fNear;            // Near plane distance
    float fFar;             // Far plane distance
    float fFov;             // Field of view (in radians)

    DWORD dwFrameCount;     // Number of animation frames
    int nFrame;             // Current frame index
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Camera_Clear(CHCamera* lpCamera);

CH_CORE_DLL_API
BOOL Camera_Load(CHCamera** lpCamera,
                char* lpName,
                DWORD dwIndex);

CH_CORE_DLL_API
BOOL Camera_Save(char* lpName, CHCamera* lpCamera, BOOL bNew);

CH_CORE_DLL_API
void Camera_Unload(CHCamera** lpCamera);

CH_CORE_DLL_API
void Camera_NextFrame(CHCamera* lpCamera, int nStep);

/*
    Build view matrix
    ----------------
    Call this when camera position or target changes
*/
CH_CORE_DLL_API
BOOL Camera_BuildView(CHCamera* lpCamera, BOOL bSet = true);

/*
    Build projection matrix
    ----------------------
    Call this when near/far planes or FOV changes
*/
CH_CORE_DLL_API
BOOL Camera_BuildProject(CHCamera* lpCamera, BOOL bSet = true);

/*
    Build orthographic projection matrix
    -----------------------------------
    Call this when switching to orthographic mode
*/
CH_CORE_DLL_API
BOOL Camera_BuildOrtho(CHCamera* lpCamera,
                      float fWidth,
                      float fHeight,
                      BOOL bSet = true);

CH_CORE_DLL_API
void Camera_Process1stRotate(CHCamera* lpCamera,
                           float fHRadian,
                           float fVRadian);

CH_CORE_DLL_API
void Camera_Process1stTranslate(CHCamera* lpCamera,
                              float fDirRadian,
                              float fStep);

CH_CORE_DLL_API
void Camera_ProcessXYTranslate(CHCamera* lpCamera,
                             float fDirRadian,
                             float fStep);

// Internal helper functions for DirectXMath compatibility
namespace CHCameraInternal {
    // Math utility functions
    XMMATRIX CreateLookAtMatrix(const XMVECTOR& from, const XMVECTOR& to, const XMVECTOR& up);
    XMMATRIX CreatePerspectiveMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
    XMMATRIX CreateOrthographicMatrix(float width, float height, float nearPlane, float farPlane);
    
    // Camera control utilities
    void RotateVector(XMVECTOR& vector, const XMVECTOR& axis, float radians);
    void ClampVerticalRotation(XMVECTOR& target, const XMVECTOR& from, float minAngle, float maxAngle);
    
    // File I/O utilities
    BOOL ReadCameraChunk(FILE* file, CHCamera* camera);
    BOOL WriteCameraChunk(FILE* file, CHCamera* camera, ChunkHeader& chunk);
}

// Compatibility types
typedef CHCamera C3Camera;

// Math conversion utilities for compatibility
namespace CHCameraMath {
    // Convert between DirectX 8 D3DXVECTOR3 and DirectXMath XMVECTOR
    inline XMVECTOR VectorFromD3DX(float x, float y, float z) {
        return XMVectorSet(x, y, z, 1.0f);
    }
    
    inline void VectorToD3DX(const XMVECTOR& v, float& x, float& y, float& z) {
        x = XMVectorGetX(v);
        y = XMVectorGetY(v);
        z = XMVectorGetZ(v);
    }
    
    // Radian conversion (maintaining compatibility with D3DXToRadian)
    inline float ToRadian(float degrees) {
        return degrees * (XM_PI / 180.0f);
    }
    
    inline float ToDegree(float radians) {
        return radians * (180.0f / XM_PI);
    }
}

#endif // _CH_camera_h_