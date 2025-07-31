#ifndef _CH_shape_h_
#define _CH_shape_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_main.h"
#include "CH_texture.h"
#include "CH_key.h"

// Shape vertex format (matches original D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define SHAPE_VERTEX_FORMAT_DESC { \
    "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }

struct CHShapeOutVertex {
    float x, y, z;      // Position
    DWORD color;        // Vertex color
    float u, v;         // Texture coordinates
};

struct CHShapeBackupInfo {
    CHShapeOutVertex* vb;
    DWORD dwSegmentCur;
    XMVECTOR last[2];   // Last two points (D3DXVECTOR3 to XMVECTOR)
    BOOL bFirst;
};

struct CHLine {
    DWORD dwVecCount;           // Number of vertices
    XMVECTOR* lpVB;            // Vertex buffer (D3DXVECTOR3 to XMVECTOR)
};

struct CHSMotion {
    DWORD dwFrames;             // Number of frames
    XMMATRIX* lpFrames;         // Motion matrices (D3DXMATRIX to XMMATRIX)
    XMMATRIX matrix;            // Current transformation matrix
    int nFrame;                 // Current frame
};

// Shape motion functions
CH_CORE_DLL_API
void SMotion_Clear(CHSMotion* lpSMotion);

CH_CORE_DLL_API
BOOL SMotion_Load(CHSMotion** lpSMotion, FILE* file);

CH_CORE_DLL_API
BOOL SMotion_LoadPack(CHSMotion** lpSMotion, HANDLE f);

CH_CORE_DLL_API
BOOL SMotion_Save(char* lpName, CHSMotion* lpSMotion, BOOL bNew);

CH_CORE_DLL_API
void SMotion_Unload(CHSMotion** lpSMotion);

struct CHShape {
    char* lpName;               // Shape name
    
    DWORD dwLineCount;          // Number of lines
    CHLine* lpLine;             // Line array
    
    char* lpTexName;            // Texture name (for plugin use)
    int nTex;                   // Texture ID
    
    CHSMotion* lpMotion;        // Motion animation data
    
    // Flash rendering data
    CHShapeOutVertex* vb;       // Vertex buffer
    DWORD dwSegment;            // Number of segments
    DWORD dwSegmentCur;         // Current segment
    
    XMVECTOR last[2];           // Last two points (D3DXVECTOR3 to XMVECTOR)
    BOOL bFirst;                // First point flag
    
    DWORD dwSmooth;             // Smoothing level
    XMVECTOR* lpSmooths0;       // Smoothing points 0 (D3DXVECTOR3 to XMVECTOR)
    XMVECTOR* lpSmooths1;       // Smoothing points 1 (D3DXVECTOR3 to XMVECTOR)
    
    // TearAir effect data
    CHComPtr<ID3D11Texture2D> pTearAirTex;     // Tear air texture
    XMVECTOR* pScreenPnt;                      // Screen points (D3DXVECTOR3 to XMVECTOR)
    RECT TearAirTexRect;                       // Current texture rect
    RECT LastTearAirTexRect;                   // Last texture rect
    
    // DirectX 11 specific data (internal use)
    CHComPtr<ID3D11Buffer> vertexBuffer;
    CHComPtr<ID3D11Buffer> indexBuffer;
    UINT vertexStride;
    UINT vertexOffset;
};

// Shape functions
CH_CORE_DLL_API
void Shape_Clear(CHShape* lpShape);

CH_CORE_DLL_API
BOOL Shape_Load(CHShape** lpShape, FILE* file, BOOL bTex = FALSE);

CH_CORE_DLL_API
BOOL Shape_LoadPack(CHShape** lpShape, HANDLE f, BOOL bTex = FALSE);

CH_CORE_DLL_API
BOOL Shape_Save(char* lpName, CHShape* lpShape, BOOL bNew);

CH_CORE_DLL_API
void Shape_Unload(CHShape** lpShape);

CH_CORE_DLL_API
void Shape_SetSegment(CHShape* lpShape, DWORD dwSegment, DWORD dwSmooth = 1);

CH_CORE_DLL_API
BOOL Shape_Draw(CHShape* lpShape, BOOL bLocal = FALSE, int nAsb = 5, int nAdb = 6);

CH_CORE_DLL_API
void Shape_Prepare();

CH_CORE_DLL_API
void Shape_ChangeTexture(CHShape* lpShape, int nTexID);

CH_CORE_DLL_API
void Shape_Muliply(CHShape* lpShape, XMMATRIX* matrix);

CH_CORE_DLL_API
void Shape_NextFrame(CHShape* lpShape, int nStep);

CH_CORE_DLL_API
void Shape_SetFrame(CHShape* lpShape, DWORD dwFrame);

CH_CORE_DLL_API
void Shape_ClearMatrix(CHShape* lpShape);

CH_CORE_DLL_API
BOOL Shape_DrawAlpha(CHShape* lpShape, BOOL bLocal);

// Internal implementation
namespace CHShapeInternal {
    // Shape processing
    void ProcessShapeLines(CHShape* shape);
    void GenerateShapeGeometry(CHShape* shape);
    void SmoothShapeLines(CHShape* shape);
    
    // Buffer management
    BOOL CreateVertexBuffer(CHShape* shape);
    BOOL CreateIndexBuffer(CHShape* shape);
    void UpdateVertexBuffer(CHShape* shape);
    void ReleaseBuffers(CHShape* shape);
    
    // Rendering utilities
    void SetupShapeRenderStates();
    BOOL RenderShape(CHShape* shape, bool enableZ, int srcBlend, int destBlend);
    
    // Motion processing
    BOOL LoadSMotionFromFile(FILE* file, CHSMotion** motion);
    BOOL LoadSMotionFromPack(HANDLE handle, CHSMotion** motion);
    void ProcessSMotionFrame(CHSMotion* motion);
    
    // TearAir effect
    void SetupTearAirEffect(CHShape* shape);
    void UpdateTearAirTexture(CHShape* shape);
    void RenderTearAirEffect(CHShape* shape);
    
    // File I/O utilities
    BOOL LoadShapeFromFile(FILE* file, CHShape** shape, bool loadTextures);
    BOOL LoadShapeFromPack(HANDLE handle, CHShape** shape, bool loadTextures);
    BOOL SaveShapeToFile(const char* filename, CHShape* shape, bool newFile);
    
    // Line processing
    void ProcessShapeLine(CHShape* shape, DWORD lineIndex);
    void OptimizeShapeLines(CHShape* shape);
    XMVECTOR InterpolateShapePoint(XMVECTOR p0, XMVECTOR p1, XMVECTOR p2, XMVECTOR p3, float t);
}

// Compatibility types
typedef CHShape C3Shape;
typedef CHLine C3Line;
typedef CHSMotion C3SMotion;
typedef CHShapeOutVertex ShapeOutVertex;
typedef CHShapeBackupInfo ShapeBackupInfo;

#endif