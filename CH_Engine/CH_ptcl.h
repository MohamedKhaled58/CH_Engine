#ifndef _CH_ptcl_h_
#define _CH_ptcl_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include "CH_texture.h"

// Particle vertex structure for rendering
struct CHPtclVertex {
    float x, y, z;      // Position
    DWORD color;        // Particle color
    float u, v;         // Texture coordinates
};

// Particle frame data
struct CHPtclFrame {
    DWORD dwCount;              // Number of particles in this frame
    XMVECTOR* lpPos;           // Particle positions
    float* lpAge;               // Particle ages
    float* lpSize;              // Particle sizes
    XMMATRIX matrix;           // Frame transformation matrix
};

// Particle system structure (maintains exact same layout as C3Ptcl)
struct CHPtcl {
    char* lpName;               // Particle system name
    CHPtclVertex* lpVB;         // Vertex buffer (CPU side)
    WORD* lpIB;                 // Index buffer (CPU side)
    int nTex;                   // Texture ID
    char* lpTexName;            // Texture name (for plugin use)
    DWORD dwCount;              // Maximum particle count
    DWORD dwRow;                // Texture rows (for animation)

    CHPtclFrame* lpPtcl;        // Frame data array
    int nFrame;                 // Current frame
    DWORD dwFrames;             // Total frames

    XMMATRIX matrix;           // Transformation matrix

    // DirectX 11 specific data (internal use)
    CHComPtr<ID3D11Buffer> vertexBuffer = nullptr;
    CHComPtr<ID3D11Buffer> indexBuffer = nullptr;
    UINT vertexStride;
    UINT vertexOffset;
};

// Function declarations (maintaining exact same signatures as original)
CH_CORE_DLL_API
void Ptcl_Clear(CHPtcl* lpPtcl);

CH_CORE_DLL_API
BOOL Ptcl_Load(CHPtcl** lpPtcl, FILE* file, BOOL bTex = FALSE);

CH_CORE_DLL_API
BOOL Ptcl_LoadPack(CHPtcl** lpPtcl, HANDLE f, BOOL bTex = FALSE);

CH_CORE_DLL_API
BOOL Ptcl_Save(char* lpName, CHPtcl* lpPtcl, BOOL bNew);

CH_CORE_DLL_API
void Ptcl_Unload(CHPtcl** lpPtcl);

CH_CORE_DLL_API
void Ptcl_Prepare();

CH_CORE_DLL_API
BOOL Ptcl_Draw(CHPtcl* lpPtcl, int nAsb = 5, int nAdb = 6);

CH_CORE_DLL_API
void Ptcl_NextFrame(CHPtcl* lpPtcl, int nStep);

CH_CORE_DLL_API
void Ptcl_SetFrame(CHPtcl* lpPtcl, DWORD dwFrame);

CH_CORE_DLL_API
void Ptcl_Muliply(CHPtcl* lpPtcl, XMMATRIX* matrix);

CH_CORE_DLL_API
void Ptcl_ClearMatrix(CHPtcl* lpPtcl);

CH_CORE_DLL_API
void Ptcl_ChangeTexture(CHPtcl* lpPtcl, int nTexID);

// Internal particle system implementation
namespace CHPtclInternal {
    // Particle simulation
    void UpdateParticles(CHPtcl* ptcl);
    void GenerateQuads(CHPtcl* ptcl);
    void SortParticlesByDepth(CHPtcl* ptcl);
    
    // Buffer management
    BOOL CreateVertexBuffer(CHPtcl* ptcl);
    BOOL CreateIndexBuffer(CHPtcl* ptcl);
    void UpdateVertexBuffer(CHPtcl* ptcl);
    void ReleaseBuffers(CHPtcl* ptcl);
    
    // Rendering
    void SetupParticleRenderStates();
    BOOL RenderParticleSystem(CHPtcl* ptcl, int srcBlend, int destBlend);
    
    // File I/O
    BOOL LoadPtclFromFile(FILE* file, CHPtcl** ptcl, bool loadTextures);
    BOOL LoadPtclFromPack(HANDLE handle, CHPtcl** ptcl, bool loadTextures);


    class ParticleShaderManager {
    public:
        HRESULT Initialize();
        void SetParticleShaders();
        void Cleanup();
    private:
        CHComPtr<ID3D11VertexShader> m_vertexShader;
        CHComPtr<ID3D11PixelShader> m_pixelShader;
        CHComPtr<ID3D11InputLayout> m_inputLayout;
    };

    extern ParticleShaderManager g_ParticleShaderManager;
}

// Compatibility types
typedef CHPtcl C3Ptcl;
typedef CHPtclVertex PtclVertex;
typedef CHPtclFrame PtclFrame;

#endif // _CH_ptcl_h_