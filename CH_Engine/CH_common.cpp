#include "CH_common.h"
#include "CH_main.h"
#include "CH_datafile.h"
#include <random>
#include <signal.h>
#include <string>

// Global file handle for compatibility
FILE* g_filetemp = nullptr;

// Global mutex for thread safety
CRITICAL_SECTION g_GlobalMutex;

// Error handling functions
void AssertDialog(const char* expression, const char* file, int line)
{
    char OutputMessage[256];
    sprintf_s(OutputMessage, 256, "Assert ( %s );\nFile: %s\nLine: %d", expression, file, line);
    ErrorMessage(OutputMessage);
}

void ErrorMessage(const char* message)
{
    switch (MessageBoxA(g_hWnd, message, "ERROR MESSAGE", MB_ABORTRETRYIGNORE))
    {
    case IDABORT:
        // Send abort signal
        raise(SIGABRT);
        // Exit if signal is ignored
        exit(3);
        break;
    case IDRETRY:
        // Trigger debugger breakpoint
        __debugbreak();
        break;
    case IDIGNORE:
        ShowWindow(GetActiveWindow(), SW_SHOW);
        break;
    }
}

void CH_OutputMessage(const char* expression, long result, const char* file, int line)
{
    std::string message = "DirectX Error!\n\n";
    message += "Expression: " + std::string(expression) + "\n";
    message += "HRESULT: 0x" + std::to_string(result) + "\n";
    message += "File: " + std::string(file) + "\n";
    message += "Line: " + std::to_string(line);
    
    ErrorMessage(message.c_str());
}

// Utility functions (maintaining exact same signatures as original)
int Random(int nMin, int nMax)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(nMin, nMax);
    return dis(gen);
}

int FloatCmp(float f0, float f1, float fDim)
{
    float diff = f0 - f1;
    if (diff < -fDim) return -1;
    if (diff > fDim) return 1;
    return 0;
}

void CutString(char* lpString, DWORD dwLevel)
{
    if (dwLevel == 0 || !lpString)
        return;

    DWORD nowS = 0;
    DWORD l;
    for (l = 0; l < strlen(lpString); l++)
    {
        if (lpString[l] == '\\')
        {
            nowS++;
            if (nowS == dwLevel)
            {
                l++;
                break;
            }
        }
    }
    
    if (nowS < dwLevel)
        return;

    DWORD n;
    for (n = 0; n < strlen(lpString) - l; n++)
        lpString[n] = lpString[n + l];

    lpString[n] = '\0';
}

// Resource management functions (maintaining exact API compatibility)
void Common_AddDnpDisperseFile(const char* name)
{
    g_objDnFile.AddDisperseFile(name);
}

FILE* Common_OpenDnp(const char* name, int& nSize)
{
    unsigned long uSize;
    FILE* fp = g_objDnFile.GetFPtr(name, uSize);
    nSize = static_cast<int>(uSize);
    if (fp)
        fseek(fp, 16, SEEK_CUR);
    return fp;
}

FILE* Common_MoveDnpFPtr(FILE* file, unsigned long usSize)
{
    if (!file)
        return nullptr;
    fseek(file, usSize, SEEK_CUR);
    return file;
}

void Common_BeforeUseDnp()
{
    g_objDnFile.BeforeUseDnFile();
}

void Common_AfterUseDnp()
{
    g_objDnFile.AfterUseDnFile();
}

FILE* Common_OpenRes(const char* name)
{
    if (g_filetemp)
        fclose(g_filetemp);
    
    g_filetemp = fopen(name, "rb");
    if (g_filetemp)
        fseek(g_filetemp, 16, SEEK_SET);
    return g_filetemp;
}

HANDLE Common_OpenResPack(const char* name, int& nSize)
{
    HANDLE f;
    DWORD id = pack_name(name);
    DWORD fid = real_name(name);
    DWORD m_Offset;
    CHDataFileIndex* pf;
    int i;
    
    // Search the WFile
    for (i = 0; i < MAXDATAFILE; i++)
    {
        if (DataFile_IsOpen(&_WDF[i], id))
        {
            pf = DataFile_SearchFile(&_WDF[i], fid);
            if (pf == nullptr)
                return INVALID_HANDLE_VALUE;
            f = DataFile_GetFileHandle(&_WDF[i]);
            m_Offset = pf->offset;
            nSize = pf->size;
            SetFilePointer(f, m_Offset + 16, 0, FILE_BEGIN);
            break;
        }
    }
    
    if (i == MAXDATAFILE)
        return INVALID_HANDLE_VALUE;
    else
        return f;
}

void Common_ClearRes(FILE* file)
{
    if (g_filetemp)
    {
        fclose(g_filetemp);
        g_filetemp = nullptr;
    }
}

void Common_GetChunk(FILE* file, ChunkHeader* chunk)
{
    fread(chunk, sizeof(ChunkHeader), 1, file);
}

void Common_SeekRes(FILE* file, int seek)
{
    fseek(file, seek, SEEK_CUR);
}

BOOL Common_IsEofRes()
{
    if (!g_filetemp)
        return TRUE;
        
    long current = ftell(g_filetemp);
    fseek(g_filetemp, 0, SEEK_END);
    long end = ftell(g_filetemp);
    fseek(g_filetemp, current, SEEK_SET);
    
    return current >= end;
}

// Matrix and math utility functions (using DirectXMath internally)
void Common_Translate(XMMATRIX* matrix, float x, float y, float z)
{
    XMMATRIX translation = XMMatrixTranslation(x, y, z);
    *matrix = XMMatrixMultiply(*matrix, translation);
}

void Common_Rotate(XMMATRIX* matrix, float x, float y, float z)
{
    XMMATRIX rotationX = XMMatrixRotationX(x);
    XMMATRIX rotationY = XMMatrixRotationY(y);
    XMMATRIX rotationZ = XMMatrixRotationZ(z);
    XMMATRIX rotation = XMMatrixMultiply(XMMatrixMultiply(rotationX, rotationY), rotationZ);
    *matrix = XMMatrixMultiply(*matrix, rotation);
}

void Common_Scale(XMMATRIX* matrix, float x, float y, float z)
{
    XMMATRIX scaling = XMMatrixScaling(x, y, z);
    *matrix = XMMatrixMultiply(*matrix, scaling);
}

void Common_Shadow(XMMATRIX* matrix,
                  XMVECTOR* lightpos,
                  XMVECTOR* planepoint,
                  XMVECTOR* planenor)
{
    // Create shadow matrix
    XMVECTOR lightPosition = *lightpos;
    XMVECTOR planePoint = *planepoint;
    XMVECTOR planeNormal = XMVector3Normalize(*planenor);
    
    // Calculate plane equation (D component)
    float D = -XMVectorGetX(XMVector3Dot(planeNormal, planePoint));
    
    // Create shadow matrix
    XMMATRIX shadowMatrix;
    float dot = XMVectorGetX(XMVector3Dot(planeNormal, lightPosition)) + D;
    
    // Fill shadow matrix
    shadowMatrix.r[0] = XMVectorSet(
        dot - XMVectorGetX(planeNormal) * XMVectorGetX(lightPosition),
        -XMVectorGetX(planeNormal) * XMVectorGetY(lightPosition),
        -XMVectorGetX(planeNormal) * XMVectorGetZ(lightPosition),
        -XMVectorGetX(planeNormal) * XMVectorGetW(lightPosition)
    );
    
    shadowMatrix.r[1] = XMVectorSet(
        -XMVectorGetY(planeNormal) * XMVectorGetX(lightPosition),
        dot - XMVectorGetY(planeNormal) * XMVectorGetY(lightPosition),
        -XMVectorGetY(planeNormal) * XMVectorGetZ(lightPosition),
        -XMVectorGetY(planeNormal) * XMVectorGetW(lightPosition)
    );
    
    shadowMatrix.r[2] = XMVectorSet(
        -XMVectorGetZ(planeNormal) * XMVectorGetX(lightPosition),
        -XMVectorGetZ(planeNormal) * XMVectorGetY(lightPosition),
        dot - XMVectorGetZ(planeNormal) * XMVectorGetZ(lightPosition),
        -XMVectorGetZ(planeNormal) * XMVectorGetW(lightPosition)
    );
    
    shadowMatrix.r[3] = XMVectorSet(
        -D * XMVectorGetX(lightPosition),
        -D * XMVectorGetY(lightPosition),
        -D * XMVectorGetZ(lightPosition),
        dot - D * XMVectorGetW(lightPosition)
    );
    
    *matrix = XMMatrixMultiply(*matrix, shadowMatrix);
}

void BuildRay(int nX, int nY, XMVECTOR* lpOrig, XMVECTOR* lpDir)
{
    // Match original algorithm exactly
    XMMATRIX matrix = g_ProjectMatrix;

    XMVECTOR vec;
    float x = (((2.0f * nX) / g_DisplayMode.Width) - 1) / XMVectorGetX(matrix.r[0]);
    float y = -(((2.0f * nY) / g_DisplayMode.Height) - 1) / XMVectorGetY(matrix.r[1]);
    float z = 1.0f;
    vec = XMVectorSet(x, y, z, 0.0f);

    matrix = g_ViewMatrix;
    XMVECTOR det;
    matrix = XMMatrixInverse(&det, matrix);

    // Manual matrix multiplication as in original
    float dirX = x * XMVectorGetX(matrix.r[0]) + y * XMVectorGetX(matrix.r[1]) + z * XMVectorGetX(matrix.r[2]);
    float dirY = x * XMVectorGetY(matrix.r[0]) + y * XMVectorGetY(matrix.r[1]) + z * XMVectorGetY(matrix.r[2]);
    float dirZ = x * XMVectorGetZ(matrix.r[0]) + y * XMVectorGetZ(matrix.r[1]) + z * XMVectorGetZ(matrix.r[2]);

    *lpDir = XMVectorSet(dirX, dirY, dirZ, 0.0f);

    // Get origin from matrix translation (row 3)
    *lpOrig = XMVectorSet(XMVectorGetX(matrix.r[3]), XMVectorGetY(matrix.r[3]), XMVectorGetZ(matrix.r[3]), 1.0f);

    // Normalize direction
    *lpDir = XMVector3Normalize(*lpDir);
}

void IntersectPlane(XMVECTOR* lpOrg,
                   XMVECTOR* lpDir,
                   XMVECTOR* lpPlaneNor,
                   XMVECTOR* lpPlaneVec,
                   XMVECTOR* lpResult)
{
    // Match original algorithm exactly
    // Calculate distance to plane (length of direction vector)
    float l = XMVectorGetX(XMVector3Length(*lpDir));
    XMVECTOR vec = XMVectorSubtract(*lpOrg, *lpPlaneVec);
    float dist = XMVectorGetX(XMVector3Dot(vec, *lpPlaneNor));
    
    // Scale direction by distance
    vec = XMVectorScale(*lpDir, dist);
    vec = XMVectorAdd(*lpOrg, vec);
    
    // Create plane and intersect line using DirectXMath equivalent
    XMVECTOR planeNormal = XMVector3Normalize(*lpPlaneNor);
    float D = -XMVectorGetX(XMVector3Dot(planeNormal, *lpPlaneVec));
    
    // Calculate intersection using plane equation
    float denom = XMVectorGetX(XMVector3Dot(planeNormal, XMVectorSubtract(vec, *lpOrg)));
    if (fabsf(denom) > 0.0001f)
    {
        float t = -(XMVectorGetX(XMVector3Dot(planeNormal, *lpOrg)) + D) / denom;
        *lpResult = XMVectorAdd(*lpOrg, XMVectorScale(XMVectorSubtract(vec, *lpOrg), t));
    }
    else
    {
        *lpResult = vec; // Fallback to calculated vec
    }
}

// Modern C++ helper utilities implementation
namespace CHUtils {

std::string WideToMultiByte(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();
        
    int size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                                    nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                         &result[0], size, nullptr, nullptr);
    return result;
}

std::wstring MultiByteToWide(const std::string& str)
{
    if (str.empty())
        return std::wstring();
        
    int size = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
                                    nullptr, 0);
    std::wstring result(size, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
                         &result[0], size);
    return result;
}

std::string GetDirectoryFromPath(const std::string& filepath)
{
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        return filepath.substr(0, pos);
    return "";
}

std::string GetFilenameFromPath(const std::string& filepath)
{
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        return filepath.substr(pos + 1);
    return filepath;
}

std::string GetExtensionFromPath(const std::string& filepath)
{
    size_t pos = filepath.find_last_of('.');
    if (pos != std::string::npos)
        return filepath.substr(pos + 1);
    return "";
}

size_t HashString(const std::string& str)
{
    return std::hash<std::string>{}(str);
}

size_t HashStringCaseInsensitive(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return std::hash<std::string>{}(lowerStr);
}

} // namespace CHUtils