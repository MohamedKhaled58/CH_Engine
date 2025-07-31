#ifndef _CH_common_h_
#define _CH_common_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#ifndef STRICT
#define STRICT
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Modern C++ includes
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <functional>

// Windows and DirectX 11 includes
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

using namespace DirectX;

#ifdef _DEBUG
    #define Assert(Expression) \
        do { \
            if (!(Expression)) { \
                AssertDialog(#Expression, __FILE__, __LINE__); \
            } \
        } while(0)
#else
    #define Assert(Expression) ((void)0)
#endif

// Modern C++ error handling and resource management
class CHException : public std::runtime_error {
public:
    explicit CHException(const std::string& message) : std::runtime_error(message) {}
    explicit CHException(const char* message) : std::runtime_error(message) {}
};

// Modern RAII-based resource management
template<typename T>
class CHComPtr {
private:
    T* ptr = nullptr;

public:
    CHComPtr() = default;
    CHComPtr(T* p) : ptr(p) {}
    
    CHComPtr(const CHComPtr& other) : ptr(other.ptr) {
        if (ptr) ptr->AddRef();
    }
    
    CHComPtr(CHComPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    ~CHComPtr() {
        if (ptr) ptr->Release();
    }
    
    CHComPtr& operator=(const CHComPtr& other) {
        if (this != &other) {
            if (ptr) ptr->Release();
            ptr = other.ptr;
            if (ptr) ptr->AddRef();
        }
        return *this;
    }
    
    CHComPtr& operator=(CHComPtr&& other) noexcept {
        if (this != &other) {
            if (ptr) ptr->Release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    T* Get() const { return ptr; }
    T** GetAddressOf() { return &ptr; }
    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }
    operator bool() const { return ptr != nullptr; }
    
    void Reset() {
        if (ptr) {
            ptr->Release();
            ptr = nullptr;
        }
    }
    
    BOOL As(CHComPtr<T>* other) const {
        return SUCCEEDED(ptr->QueryInterface(__uuidof(T), reinterpret_cast<void**>(other->GetAddressOf())));
    }
};

// Modern equivalent of old macros
#define SafeRelease(ptr) if (ptr) { ptr->Release(); ptr = nullptr; }

template<typename T>
void SafeDelete(std::unique_ptr<T>& ptr) {
    ptr.reset();
}

template<typename T>
void SafeDelete(T*& ptr) {
    delete ptr;
    ptr = nullptr;
}

template<typename T>
void SafeDeleteArray(T*& ptr) {
    delete[] ptr;
    ptr = nullptr;
}

#define CH_TRY(Expression) \
    do { \
        BOOL success = SUCCEEDED(Expression); \
        if (!success) { \
            CH_OutputMessage(#Expression, FALSE, __FILE__, __LINE__); \
        } \
    } while(0)

// Error handling functions
CH_CORE_DLL_API void AssertDialog(const char* expression, const char* file, int line);
CH_CORE_DLL_API void ErrorMessage(const char* message);
CH_CORE_DLL_API void CH_OutputMessage(const char* expression, BOOL result, const char* file, int line);

// Utility functions (maintaining exact same signatures as original)
CH_CORE_DLL_API int Random(int nMin, int nMax);
CH_CORE_DLL_API int FloatCmp(float f0, float f1, float fDim = 0.0001f);
CH_CORE_DLL_API void CutString(char* lpString, DWORD dwLevel);

// Chunk header structure (maintaining exact compatibility)
struct ChunkHeader {
    BYTE byChunkID[4];      // Chunk ID
    DWORD dwChunkSize;      // Chunk size
};

// File handle management
extern CH_CORE_DLL_API FILE* g_filetemp;

// Resource management functions (maintaining exact API compatibility)
CH_CORE_DLL_API void Common_AddDnpDisperseFile(const char* name);
CH_CORE_DLL_API FILE* Common_OpenDnp(const char* name, int& nSize);
CH_CORE_DLL_API FILE* Common_MoveDnpFPtr(FILE* file, unsigned long usSize);
CH_CORE_DLL_API void Common_BeforeUseDnp();
CH_CORE_DLL_API void Common_AfterUseDnp();
CH_CORE_DLL_API FILE* Common_OpenRes(const char* name);
CH_CORE_DLL_API HANDLE Common_OpenResPack(const char* name, int& nSize);
CH_CORE_DLL_API void Common_ClearRes(FILE* file);
CH_CORE_DLL_API void Common_GetChunk(FILE* file, ChunkHeader* chunk);
CH_CORE_DLL_API void Common_SeekRes(FILE* file, int seek);
CH_CORE_DLL_API BOOL Common_IsEofRes();

// Matrix and math utility functions (maintaining exact API but using DirectXMath internally)
CH_CORE_DLL_API void Common_Translate(XMMATRIX* matrix, float x, float y, float z);
CH_CORE_DLL_API void Common_Rotate(XMMATRIX* matrix, float x, float y, float z);
CH_CORE_DLL_API void Common_Scale(XMMATRIX* matrix, float x, float y, float z);
CH_CORE_DLL_API void Common_Shadow(XMMATRIX* matrix, 
                                  XMVECTOR* lightpos,
                                  XMVECTOR* planepoint,
                                  XMVECTOR* planenor);

// Ray and intersection utilities
CH_CORE_DLL_API void BuildRay(int nX, int nY, XMVECTOR* lpOrig, XMVECTOR* lpDir);
CH_CORE_DLL_API void IntersectPlane(XMVECTOR* lpOrg,
                                   XMVECTOR* lpDir,
                                   XMVECTOR* lpPlaneNor,
                                   XMVECTOR* lpPlaneVec,
                                   XMVECTOR* lpResult);

// Thread safety
extern CH_CORE_DLL_API CRITICAL_SECTION g_GlobalMutex;

// Modern C++ helper utilities
namespace CHUtils {
    // String conversion utilities
    std::string WideToMultiByte(const std::wstring& wstr);
    std::wstring MultiByteToWide(const std::string& str);
    
    // File path utilities
    std::string GetDirectoryFromPath(const std::string& filepath);
    std::string GetFilenameFromPath(const std::string& filepath);
    std::string GetExtensionFromPath(const std::string& filepath);
    
    // Hash utilities for resource management
    size_t HashString(const std::string& str);
    size_t HashStringCaseInsensitive(const std::string& str);
}

#endif // _CH_common_h_