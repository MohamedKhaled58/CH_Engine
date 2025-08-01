// CH_constants.h - All missing CH_ constants definitions
// Add this to CH_main.h or create as separate header

#ifndef _CH_CONSTANTS_H_
#define _CH_CONSTANTS_H_

#include <windows.h>  // For DWORD, BOOL, etc.

// DirectX 8 style render state constants (translated to DirectX 11 internally)
enum CHRenderStateType {
    CH_RS_ZENABLE = 7,
    CH_RS_FILLMODE = 8,
    CH_RS_SHADEMODE = 9,
    CH_RS_ZWRITEENABLE = 14,
    CH_RS_ALPHATESTENABLE = 15,
    CH_RS_LASTPIXEL = 16,
    CH_RS_SRCBLEND = 19,
    CH_RS_DESTBLEND = 20,
    CH_RS_CULLMODE = 22,
    CH_RS_ZFUNC = 23,
    CH_RS_ALPHAREF = 24,
    CH_RS_ALPHAFUNC = 25,
    CH_RS_DITHERENABLE = 26,
    CH_RS_ALPHABLENDENABLE = 27,
    CH_RS_FOGENABLE = 28,
    CH_RS_SPECULARENABLE = 29,
    CH_RS_FOGCOLOR = 34,
    CH_RS_FOGTABLEMODE = 35,
    CH_RS_FOGSTART = 36,
    CH_RS_FOGEND = 37,
    CH_RS_FOGDENSITY = 38,
    CH_RS_RANGEFOGENABLE = 48,
    CH_RS_STENCILENABLE = 52,
    CH_RS_STENCILFAIL = 53,
    CH_RS_STENCILZFAIL = 54,
    CH_RS_STENCILPASS = 55,
    CH_RS_STENCILFUNC = 56,
    CH_RS_STENCILREF = 57,
    CH_RS_STENCILMASK = 58,
    CH_RS_STENCILWRITEMASK = 59,
    CH_RS_TEXTUREFACTOR = 60,
    CH_RS_WRAP0 = 128,
    CH_RS_WRAP1 = 129,
    CH_RS_WRAP2 = 130,
    CH_RS_WRAP3 = 131,
    CH_RS_WRAP4 = 132,
    CH_RS_WRAP5 = 133,
    CH_RS_WRAP6 = 134,
    CH_RS_WRAP7 = 135,
    CH_RS_CLIPPING = 136,
    CH_RS_LIGHTING = 137,
    CH_RS_AMBIENT = 139,
    CH_RS_FOGVERTEXMODE = 140,
    CH_RS_COLORVERTEX = 141,
    CH_RS_LOCALVIEWER = 142,
    CH_RS_NORMALIZENORMALS = 143,
    CH_RS_DIFFUSEMATERIALSOURCE = 145,
    CH_RS_SPECULARMATERIALSOURCE = 146,
    CH_RS_AMBIENTMATERIALSOURCE = 147,
    CH_RS_EMISSIVEMATERIALSOURCE = 148,
    CH_RS_VERTEXBLEND = 151,
    CH_RS_CLIPPLANEENABLE = 152,
    CH_RS_POINTSIZE = 154,
    CH_RS_POINTSIZE_MIN = 155,
    CH_RS_POINTSPRITEENABLE = 156,
    CH_RS_POINTSCALEENABLE = 157,
    CH_RS_POINTSCALE_A = 158,
    CH_RS_POINTSCALE_B = 159,
    CH_RS_POINTSCALE_C = 160,
    CH_RS_MULTISAMPLEANTIALIAS = 161,
    CH_RS_MULTISAMPLEMASK = 162,
    CH_RS_PATCHEDGESTYLE = 163,
    CH_RS_DEBUGMONITORTOKEN = 165,
    CH_RS_POINTSIZE_MAX = 166,
    CH_RS_INDEXEDVERTEXBLENDENABLE = 167,
    CH_RS_COLORWRITEENABLE = 168,
    CH_RS_TWEENFACTOR = 170,
    CH_RS_BLENDOP = 171,
    CH_RS_POSITIONDEGREE = 172,
    CH_RS_NORMALDEGREE = 173,
    CH_RS_SCISSORTESTENABLE = 174,
    CH_RS_SLOPESCALEDEPTHBIAS = 175,
    CH_RS_ANTIALIASEDLINEENABLE = 176,
    CH_RS_MINTESSELLATIONLEVEL = 178,
    CH_RS_MAXTESSELLATIONLEVEL = 179,
    CH_RS_ADAPTIVETESS_X = 180,
    CH_RS_ADAPTIVETESS_Y = 181,
    CH_RS_ADAPTIVETESS_Z = 182,
    CH_RS_ADAPTIVETESS_W = 183,
    CH_RS_ENABLEADAPTIVETESSELLATION = 184,
    CH_RS_TWOSIDEDSTENCILMODE = 185,
    CH_RS_CCW_STENCILFAIL = 186,
    CH_RS_CCW_STENCILZFAIL = 187,
    CH_RS_CCW_STENCILPASS = 188,
    CH_RS_CCW_STENCILFUNC = 189,
    CH_RS_COLORWRITEENABLE1 = 190,
    CH_RS_COLORWRITEENABLE2 = 191,
    CH_RS_COLORWRITEENABLE3 = 192,
    CH_RS_BLENDFACTOR = 193,
    CH_RS_SRGBWRITEENABLE = 194,
    CH_RS_DEPTHBIAS = 195,
    CH_RS_WRAP8 = 198,
    CH_RS_WRAP9 = 199,
    CH_RS_WRAP10 = 200,
    CH_RS_WRAP11 = 201,
    CH_RS_WRAP12 = 202,
    CH_RS_WRAP13 = 203,
    CH_RS_WRAP14 = 204,
    CH_RS_WRAP15 = 205,
    CH_RS_SEPARATEALPHABLENDENABLE = 206,
    CH_RS_SRCBLENDALPHA = 207,
    CH_RS_DESTBLENDALPHA = 208,
    CH_RS_BLENDOPALPHA = 209,

    // Custom additions for compatibility
    CH_RS_EDGEANTIALIAS = 300,
    CH_RS_SOFTWAREVERTEXPROCESSING = 301
};

// Texture stage state constants
enum CHTextureStageStateType {
    CH_TSS_COLOROP = 1,
    CH_TSS_COLORARG1 = 2,
    CH_TSS_COLORARG2 = 3,
    CH_TSS_ALPHAOP = 4,
    CH_TSS_ALPHAARG1 = 5,
    CH_TSS_ALPHAARG2 = 6,
    CH_TSS_BUMPENVMAT00 = 7,
    CH_TSS_BUMPENVMAT01 = 8,
    CH_TSS_BUMPENVMAT10 = 9,
    CH_TSS_BUMPENVMAT11 = 10,
    CH_TSS_TEXCOORDINDEX = 11,
    CH_TSS_BUMPENVLSCALE = 22,
    CH_TSS_BUMPENVLOFFSET = 23,
    CH_TSS_TEXTURETRANSFORMFLAGS = 24,
    CH_TSS_COLORARG0 = 26,
    CH_TSS_ALPHAARG0 = 27,
    CH_TSS_RESULTARG = 28,
    CH_TSS_CONSTANT = 32,

    // Sampler state constants (for texture filtering)
    CH_TSS_ADDRESSU = 13,
    CH_TSS_ADDRESSV = 14,
    CH_TSS_BORDERCOLOR = 15,
    CH_TSS_MAGFILTER = 16,
    CH_TSS_MINFILTER = 17,
    CH_TSS_MIPFILTER = 18,
    CH_TSS_MIPMAPLODBIAS = 19,
    CH_TSS_MAXMIPLEVEL = 20,
    CH_TSS_MAXANISOTROPY = 21,
    CH_TSS_ADDRESSW = 25
};

// Cull mode constants  
enum CHCullMode {
    CH_CULL_NONE = 1,
    CH_CULL_CW = 2,
    CH_CULL_CCW = 3
};

// Z-buffer comparison functions
enum CHCompareFunc {
    CH_CMP_NEVER = 1,
    CH_CMP_LESS = 2,
    CH_CMP_EQUAL = 3,
    CH_CMP_LESSEQUAL = 4,
    CH_CMP_GREATER = 5,
    CH_CMP_NOTEQUAL = 6,
    CH_CMP_GREATEREQUAL = 7,
    CH_CMP_ALWAYS = 8
};

// Blend mode constants
enum CHBlend {
    CH_BLEND_ZERO = 1,
    CH_BLEND_ONE = 2,
    CH_BLEND_SRCCOLOR = 3,
    CH_BLEND_INVSRCCOLOR = 4,
    CH_BLEND_SRCALPHA = 5,
    CH_BLEND_INVSRCALPHA = 6,
    CH_BLEND_DESTALPHA = 7,
    CH_BLEND_INVDESTALPHA = 8,
    CH_BLEND_DESTCOLOR = 9,
    CH_BLEND_INVDESTCOLOR = 10,
    CH_BLEND_SRCALPHASAT = 11,
    CH_BLEND_BOTHSRCALPHA = 12,
    CH_BLEND_BOTHINVSRCALPHA = 13,
    CH_BLEND_BLENDFACTOR = 14,
    CH_BLEND_INVBLENDFACTOR = 15
};

// Texture filter constants
enum CHTextureFilter {
    CH_TEXF_NONE = 0,
    CH_TEXF_POINT = 1,
    CH_TEXF_LINEAR = 2,
    CH_TEXF_ANISOTROPIC = 3,
    CH_TEXF_PYRAMIDALQUAD = 6,
    CH_TEXF_GAUSSIANQUAD = 7
};

// Texture operation constants
enum CHTextureOp {
    CH_TOP_DISABLE = 1,
    CH_TOP_SELECTARG1 = 2,
    CH_TOP_SELECTARG2 = 3,
    CH_TOP_MODULATE = 4,
    CH_TOP_MODULATE2X = 5,
    CH_TOP_MODULATE4X = 6,
    CH_TOP_ADD = 7,
    CH_TOP_ADDSIGNED = 8,
    CH_TOP_ADDSIGNED2X = 9,
    CH_TOP_SUBTRACT = 10,
    CH_TOP_ADDSMOOTH = 11,
    CH_TOP_BLENDDIFFUSEALPHA = 12,
    CH_TOP_BLENDTEXTUREALPHA = 13,
    CH_TOP_BLENDFACTORALPHA = 14,
    CH_TOP_BLENDTEXTUREALPHAPM = 15,
    CH_TOP_BLENDCURRENTALPHA = 16,
    CH_TOP_PREMODULATE = 17,
    CH_TOP_MODULATEALPHA_ADDCOLOR = 18,
    CH_TOP_MODULATECOLOR_ADDALPHA = 19,
    CH_TOP_MODULATEINVALPHA_ADDCOLOR = 20,
    CH_TOP_MODULATEINVCOLOR_ADDALPHA = 21,
    CH_TOP_BUMPENVMAP = 22,
    CH_TOP_BUMPENVMAPLUMINANCE = 23,
    CH_TOP_DOTPRODUCT3 = 24,
    CH_TOP_MULTIPLYADD = 25,
    CH_TOP_LERP = 26
};

// Texture argument constants
enum CHTextureArg {
    CH_TA_SELECTMASK = 0x0000000f,
    CH_TA_DIFFUSE = 0x00000000,
    CH_TA_CURRENT = 0x00000001,
    CH_TA_TEXTURE = 0x00000002,
    CH_TA_TFACTOR = 0x00000003,
    CH_TA_SPECULAR = 0x00000004,
    CH_TA_TEMP = 0x00000005,
    CH_TA_CONSTANT = 0x00000006,
    CH_TA_COMPLEMENT = 0x00000010,
    CH_TA_ALPHAREPLICATE = 0x00000020
};

// Shade mode constants
enum CHShadeMode {
    CH_SHADE_FLAT = 1,
    CH_SHADE_GOURAUD = 2,
    CH_SHADE_PHONG = 3
};

// Fill mode constants  
enum CHFillMode {
    CH_FILL_POINT = 1,
    CH_FILL_WIREFRAME = 2,
    CH_FILL_SOLID = 3
};

// Fog mode constants
enum CHFogMode {
    CH_FOG_NONE = 0,
    CH_FOG_EXP = 1,
    CH_FOG_EXP2 = 2,
    CH_FOG_LINEAR = 3
};

// Material color source constants
enum CHMaterialColorSource {
    CH_MCS_MATERIAL = 0,
    CH_MCS_COLOR1 = 1,
    CH_MCS_COLOR2 = 2
};

// Blend operation constants
enum CHBlendOp {
    CH_BLENDOP_ADD = 1,
    CH_BLENDOP_SUBTRACT = 2,
    CH_BLENDOP_REVSUBTRACT = 3,
    CH_BLENDOP_MIN = 4,
    CH_BLENDOP_MAX = 5
};

// Stencil operation constants
enum CHStencilOp {
    CH_STENCILOP_KEEP = 1,
    CH_STENCILOP_ZERO = 2,
    CH_STENCILOP_REPLACE = 3,
    CH_STENCILOP_INCRSAT = 4,
    CH_STENCILOP_DECRSAT = 5,
    CH_STENCILOP_INVERT = 6,
    CH_STENCILOP_INCR = 7,
    CH_STENCILOP_DECR = 8
};

// Texture address mode constants
enum CHTextureAddress {
    CH_TADDRESS_WRAP = 1,
    CH_TADDRESS_MIRROR = 2,
    CH_TADDRESS_CLAMP = 3,
    CH_TADDRESS_BORDER = 4,
    CH_TADDRESS_MIRRORONCE = 5
};

// Vertex processing constants
enum CHVertexProcessing {
    CH_VP_SOFTWARE = 0,
    CH_VP_HARDWARE = 1,
    CH_VP_MIXED = 2
};

// Color constants (ARGB format)
#define CH_COLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define CH_COLOR_RGBA(r,g,b,a) CH_COLOR_ARGB(a,r,g,b)
#define CH_COLOR_XRGB(r,g,b)   CH_COLOR_ARGB(0xff,r,g,b)

#define CH_COLOR_XYUV(y,u,v)   CH_COLOR_ARGB(0xff,y,u,v)
#define CH_COLOR_AYUV(a,y,u,v) CH_COLOR_ARGB(a,y,u,v)

// Standard colors
#define CH_COLOR_BLACK      0xFF000000
#define CH_COLOR_WHITE      0xFFFFFFFF
#define CH_COLOR_RED        0xFFFF0000
#define CH_COLOR_GREEN      0xFF00FF00
#define CH_COLOR_BLUE       0xFF0000FF
#define CH_COLOR_YELLOW     0xFFFFFF00
#define CH_COLOR_MAGENTA    0xFFFF00FF
#define CH_COLOR_CYAN       0xFF00FFFF

// Vertex format constants (for compatibility)
#define CH_FVF_RESERVED0        0x001
#define CH_FVF_POSITION_MASK    0x400E
#define CH_FVF_XYZ              0x002
#define CH_FVF_XYZRHW           0x004
#define CH_FVF_XYZB1            0x006
#define CH_FVF_XYZB2            0x008
#define CH_FVF_XYZB3            0x00a
#define CH_FVF_XYZB4            0x00c
#define CH_FVF_XYZB5            0x00e
#define CH_FVF_XYZW             0x4002

#define CH_FVF_NORMAL           0x010
#define CH_FVF_PSIZE            0x020
#define CH_FVF_DIFFUSE          0x040
#define CH_FVF_SPECULAR         0x080

#define CH_FVF_TEXCOUNT_MASK    0xf00
#define CH_FVF_TEXCOUNT_SHIFT   8
#define CH_FVF_TEX0             0x000
#define CH_FVF_TEX1             0x100
#define CH_FVF_TEX2             0x200
#define CH_FVF_TEX3             0x300
#define CH_FVF_TEX4             0x400
#define CH_FVF_TEX5             0x500
#define CH_FVF_TEX6             0x600
#define CH_FVF_TEX7             0x700
#define CH_FVF_TEX8             0x800

#define CH_FVF_LASTBETA_UBYTE4   0x1000
#define CH_FVF_LASTBETA_D3DCOLOR 0x8000

// Common vertex formats
#define CH_FVF_VERTEX           (CH_FVF_XYZ | CH_FVF_NORMAL | CH_FVF_TEX1)
#define CH_FVF_LVERTEX          (CH_FVF_XYZ | CH_FVF_RESERVED0 | CH_FVF_DIFFUSE | CH_FVF_SPECULAR | CH_FVF_TEX1)
#define CH_FVF_TLVERTEX         (CH_FVF_XYZRHW | CH_FVF_DIFFUSE | CH_FVF_SPECULAR | CH_FVF_TEX1)

// Sprite vertex format (for sprite rendering)
#define SPRITE_VERTEX           (CH_FVF_XYZRHW | CH_FVF_DIFFUSE | CH_FVF_TEX1)

// Maximum values
#define CH_MAX_TEXTURE_STAGES   8
#define CH_MAX_SIMULTANEOUS_RENDERTARGETS 4
#define CH_MAX_VERTEX_STREAMS   16
#define CH_MAX_CLIP_PLANES      6

// Primitive types (for DrawPrimitive calls)
enum CHPrimitiveType {
    CH_PT_POINTLIST = 1,
    CH_PT_LINELIST = 2,
    CH_PT_LINESTRIP = 3,
    CH_PT_TRIANGLELIST = 4,
    CH_PT_TRIANGLESTRIP = 5,
    CH_PT_TRIANGLEFAN = 6
};

// Usage constants for vertex/index buffers
enum CHUsage {
    CH_USAGE_RENDERTARGET = 0x00000001L,
    CH_USAGE_DEPTHSTENCIL = 0x00000002L,
    CH_USAGE_DYNAMIC = 0x00000200L,
    CH_USAGE_NONSECURE = 0x00800000L,
    CH_USAGE_AUTOGENMIPMAP = 0x00000400L,
    CH_USAGE_DMAP = 0x00004000L,
    CH_USAGE_QUERY_LEGACYBUMPMAP = 0x00008000L,
    CH_USAGE_QUERY_SRGBREAD = 0x00010000L,
    CH_USAGE_QUERY_FILTER = 0x00020000L,
    CH_USAGE_QUERY_SRGBWRITE = 0x00040000L,
    CH_USAGE_QUERY_POSTPIXELSHADER_BLENDING = 0x00080000L,
    CH_USAGE_QUERY_VERTEXTEXTURE = 0x00100000L,
    CH_USAGE_QUERY_WRAPANDMIP = 0x00200000L,
    CH_USAGE_WRITEONLY = 0x00000008L,
    CH_USAGE_SOFTWAREPROCESSING = 0x00000010L,
    CH_USAGE_DONOTCLIP = 0x00000020L,
    CH_USAGE_POINTS = 0x00000040L,
    CH_USAGE_RTPATCHES = 0x00000080L,
    CH_USAGE_NPATCHES = 0x00000100L
};

// Lock flags for vertex/index buffers
enum CHLock {
    CH_LOCK_READONLY = 0x00000010L,
    CH_LOCK_DISCARD = 0x00002000L,
    CH_LOCK_NOOVERWRITE = 0x00001000L,
    CH_LOCK_NOSYSLOCK = 0x00000800L,
    CH_LOCK_DONOTWAIT = 0x00004000L,
    CH_LOCK_NO_DIRTY_UPDATE = 0x00008000L
};

#endif // _CH_CONSTANTS_H_