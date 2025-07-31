#ifndef _CH_scene_h_
#define _CH_scene_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include "CH_texture.h"

// Scene vertex definition (maintaining exact same layout as original)
struct CHSceneVertex {
    float x, y, z;      // Position
    float nx, ny, nz;   // Normal
    float u, v;         // Texture coordinates
    float lu, lv;       // Lightmap coordinates
};

struct CHSceneEdge {
    DWORD dwStartVec;
    DWORD dwEndVec;
};

// Scene structure (maintaining exact same layout as C3Scene)
struct CHScene {
    char* lpName;                       // Scene name

    DWORD dwVecCount;                   // Vertex count
    CHSceneVertex* lpVB;                // Vertex buffer
    DWORD dwTriCount;                   // Triangle count
    WORD* lpIB;                         // Index buffer

    char* lpTexName;                    // Texture name (for plugin use)
    int nTex;                           // Texture ID

    char* lplTexName;                   // Lightmap name (for plugin use)
    int nlTex;                          // Lightmap texture ID

    DWORD dwFrameCount;                 // Animation frame count
    XMMATRIX* lpFrame;                  // Frame matrices
    int nFrame;                         // Current frame

    XMMATRIX matrix;                    // Transformation matrix

    // DirectX 11 specific data (internal use)
    CHComPtr<ID3D11Buffer> vertexBuffer;
    CHComPtr<ID3D11Buffer> indexBuffer;
    UINT vertexStride;
    UINT vertexOffset;
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Scene_Clear(CHScene* lpScene);

CH_CORE_DLL_API
BOOL Scene_Load(CHScene** lpScene,
               char* lpName,
               DWORD dwIndex);

CH_CORE_DLL_API
BOOL Scene_Save(char* lpName, CHScene* lpScene, BOOL bNew);

CH_CORE_DLL_API
void Scene_Unload(CHScene** lpScene);

// Optimization function
CH_CORE_DLL_API
BOOL Scene_Optimize(CHScene* lpScene);

CH_CORE_DLL_API
void Scene_Prepare();

CH_CORE_DLL_API
BOOL Scene_Draw(CHScene* lpScene);

CH_CORE_DLL_API
void Scene_NextFrame(CHScene* lpScene, int nStep);

CH_CORE_DLL_API
void Scene_Muliply(CHScene* lpScene, XMMATRIX* matrix);

// Internal DirectX 11 implementation helpers
namespace CHSceneInternal {
    // Vertex format constants
    const UINT SCENE_VERTEX_SIZE = sizeof(CHSceneVertex);
    
    // Scene rendering setup
    void SetupSceneRenderStates();
    void SetupLightmapRenderStates();
    void DisableLightmapRenderStates();
    
    // Buffer management
    HRESULT CreateVertexBuffer(CHScene* scene);
    HRESULT CreateIndexBuffer(CHScene* scene);
    void ReleaseBuffers(CHScene* scene);
    
    // Rendering utilities
    HRESULT RenderScene(CHScene* scene);
    bool ShouldUseAlphaBlending(CHTexture* texture);
}

// Compatibility types
typedef CHScene C3Scene;
typedef CHSceneVertex SceneVertex;
typedef CHSceneEdge SceneEdge;

// Vertex format constant for compatibility
#define SCENE_VERTEX (D3D11_INPUT_ELEMENT_DESC{ \
    "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 })

#endif // _CH_scene_h_