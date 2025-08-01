#ifndef _CH_phy_h_
#define _CH_phy_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include "CH_texture.h"
#include "CH_key.h"
#include "CH_main.h"

// Physics/Skeletal animation output vertex (for rendering)
struct CHPhyOutVertex {
    float x, y, z;      // Position
    DWORD color;        // Vertex color
    float u, v;         // Texture coordinates
};

// Physics/Skeletal animation constants
#define CH_BONE_MAX     2
#define CH_MORPH_MAX    4

// Physics vertex (with bone weights and morph targets)
struct CHPhyVertex {
    XMVECTOR pos[CH_MORPH_MAX];     // Morph target positions
    float u, v;                     // Texture coordinates
    DWORD color;                    // Vertex color
    DWORD index[CH_BONE_MAX];       // Bone indices
    float weight[CH_BONE_MAX];      // Bone weights
};

// Motion keyframe
struct CHKeyFrame {
    DWORD pos;                      // Frame position
    XMMATRIX* matrix;              // Transformation matrix
};

// Motion animation data
struct CHMotion {
    DWORD dwBoneCount;              // Number of bones
    DWORD dwFrames;                 // Number of frames

    DWORD dwKeyFrames;              // Number of keyframes
    CHKeyFrame* lpKeyFrame;         // Keyframe array

    XMMATRIX* matrix;              // Bone matrices

    DWORD dwMorphCount;             // Number of morph targets
    float* lpMorph;                 // Morph weights
    int nFrame;                     // Current frame
};

// Motion function declarations
CH_CORE_DLL_API
void Motion_Clear(CHMotion* lpMotion);

CH_CORE_DLL_API
BOOL Motion_Load(CHMotion** lpMotion, FILE* file);

CH_CORE_DLL_API
BOOL Motion_LoadPack(CHMotion** lpMotion, HANDLE f);

CH_CORE_DLL_API
BOOL Motion_Save(char* lpName, CHMotion* lpMotion, BOOL bNew);

CH_CORE_DLL_API
void Motion_Unload(CHMotion** lpMotion);

CH_CORE_DLL_API
void Motion_GetMatrix(CHMotion* lpMotion, DWORD dwBone, XMMATRIX* lpMatrix);

// Physics object structure (skeletal animated mesh)
struct CHPhy {
    char* lpName;                   // Object name

    DWORD dwBlendCount;             // Number of bones affecting each vertex

    DWORD dwNVecCount;              // Normal vertex count
    DWORD dwAVecCount;              // Alpha vertex count
    CHPhyVertex* lpVB;              // Vertex buffer (CPU side)
    
    DWORD dwNTriCount;              // Normal triangle count
    DWORD dwATriCount;              // Alpha triangle count
    WORD* lpIB;                     // Index buffer (CPU side)

    char* lpTexName;                // Texture name (for plugin use)
    int nTex;                       // Primary texture ID
    int nTex2;                      // Secondary texture ID
    XMVECTOR bboxMin, bboxMax;      // Bounding box

    CHMotion* lpMotion;             // Animation data

    float fA, fR, fG, fB;           // Color modulation (Alpha, Red, Green, Blue)

    CHKey Key;                      // Animation keys
    BOOL bDraw;                     // Draw flag

    DWORD dwTexRow;                 // Texture row (for texture atlases)

    XMMATRIX InitMatrix;           // Initial transformation matrix

    XMFLOAT2 uvstep;               // UV animation step

    // DirectX 11 specific data (internal use)
    CHComPtr<ID3D11Buffer> normalVertexBuffer = nullptr;
    CHComPtr<ID3D11Buffer> alphaVertexBuffer = nullptr;
    CHComPtr<ID3D11Buffer> normalIndexBuffer = nullptr;
    CHComPtr<ID3D11Buffer> alphaIndexBuffer = nullptr;
    CHComPtr<ID3D11Buffer> boneMatrixBuffer = nullptr;
    
    CHPhyOutVertex* lpOutVB;        // Processed output vertices
    UINT normalVertexStride;
    UINT alphaVertexStride;
    UINT vertexOffset;
};

// Physics function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Phy_Clear(CHPhy* lpPhy);

CH_CORE_DLL_API
BOOL Phy_Load(CHPhy** lpPhy, FILE* file, BOOL bTex = FALSE);

CH_CORE_DLL_API
BOOL Phy_LoadPack(CHPhy** lpPhy, HANDLE f, BOOL bTex = FALSE);

CH_CORE_DLL_API
BOOL Phy_Save(char* lpName, CHPhy* lpPhy, BOOL bNew);

CH_CORE_DLL_API
void Phy_Unload(CHPhy** lpPhy);

CH_CORE_DLL_API
void Phy_Prepare();

CH_CORE_DLL_API
BOOL Phy_Calculate(CHPhy* lpPhy);

CH_CORE_DLL_API
BOOL Phy_DrawNormal(CHPhy* lpPhy);

CH_CORE_DLL_API
BOOL Phy_DrawAlpha(CHPhy* lpPhy, BOOL bZ = FALSE, int nAsb = 5, int nAdb = 6);

CH_CORE_DLL_API
void Phy_NextFrame(CHPhy* lpPhy, int nStep);

CH_CORE_DLL_API
void Phy_SetFrame(CHPhy* lpPhy, DWORD dwFrame);

CH_CORE_DLL_API
void Phy_Muliply(CHPhy* lpPhy, int nBoneIndex, XMMATRIX* matrix);

CH_CORE_DLL_API
void Phy_SetColor(CHPhy* lpPhy,
                 float alpha,
                 float red,
                 float green,
                 float blue);

CH_CORE_DLL_API
void Phy_ClearMatrix(CHPhy* lpPhy);

CH_CORE_DLL_API
void Phy_ChangeTexture(CHPhy* lpPhy, int nTexID, int nTexID2 = 0);

// Internal DirectX 11 implementation helpers
namespace CHPhyInternal {
    // Vertex format constants
    const UINT PHY_OUT_VERTEX_SIZE = sizeof(CHPhyOutVertex);
    
    // Skeletal animation utilities
    void ProcessVertexBlending(CHPhy* phy);
    void InterpolateKeyframes(CHMotion* motion, float frame, XMMATRIX* outMatrices);
    void ApplyBoneTransforms(CHPhy* phy);
    void ApplyMorphTargets(CHPhy* phy);
    
    // Buffer management
    BOOL CreateVertexBuffers(CHPhy* phy);
    BOOL CreateIndexBuffers(CHPhy* phy);
    BOOL CreateBoneMatrixBuffer(CHPhy* phy);
    void UpdateVertexBuffer(CHPhy* phy, bool isAlpha);
    void UpdateBoneMatrices(CHPhy* phy);
    void ReleaseBuffers(CHPhy* phy);
    
    // Rendering utilities
    BOOL RenderNormalMesh(CHPhy* phy);
    BOOL RenderAlphaMesh(CHPhy* phy, bool enableZ, int srcBlend, int destBlend);
    void SetupPhyRenderStates();
    
    // Motion processing
    BOOL LoadMotionFromFile(FILE* file, CHMotion** motion);
    BOOL LoadMotionFromPack(HANDLE handle, CHMotion** motion);
    void ProcessMotionKeyframes(CHMotion* motion);
    
    // File I/O utilities
    BOOL LoadPhyFromFile(FILE* file, CHPhy** phy, bool loadTextures);
    BOOL LoadPhyFromPack(HANDLE handle, CHPhy** phy, bool loadTextures);
    BOOL SavePhyToFile(const char* filename, CHPhy* phy, bool newFile);
    
    // Shader management for skeletal animation
    class PhyShaderManager {
    private:
        CHComPtr<ID3D11VertexShader> m_skeletalVertexShader;
        CHComPtr<ID3D11PixelShader> m_normalPixelShader;
        CHComPtr<ID3D11PixelShader> m_alphaPixelShader;
        CHComPtr<ID3D11InputLayout> m_phyInputLayout;
        CHComPtr<ID3D11Buffer> m_boneConstantBuffer;
        
    public:
        BOOL Initialize();
        void SetSkeletalShaders();
        void SetNormalShaders();
        void SetAlphaShaders();
        void UpdateBoneMatrices(const XMMATRIX* boneMatrices, UINT boneCount);
        void Cleanup();
    };
    
    extern PhyShaderManager g_PhyShaderManager;
}

// Compatibility types
typedef CHPhy C3Phy;
typedef CHMotion C3Motion;
typedef CHKeyFrame C3KeyFrame;
typedef CHPhyVertex PhyVertex;
typedef CHPhyOutVertex PhyOutVertex;

// Constants for compatibility
#define _BONE_MAX_ CH_BONE_MAX
#define _MORPH_MAX_ CH_MORPH_MAX

// Vertex format constant for compatibility
#define PHY_OUT_VERTEX_FORMAT_DESC { \
    "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "BLENDINDICES", 0, DXGI_FORMAT_R32G32_UINT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, \
    { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }

#endif // _CH_phy_h_