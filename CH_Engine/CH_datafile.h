#ifndef _CH_datafile_h_
#define _CH_datafile_h_

#ifdef CH_CORE_DLL_EXPORTS
#define CH_CORE_DLL_API __declspec(dllexport)
#else
#define CH_CORE_DLL_API __declspec(dllimport)
#endif

#include "CH_common.h"
#include <mutex>
#include <unordered_map>

#define MAXDATAFILE 16

struct CHDataFileIndex {
    DWORD uid;
    DWORD offset;
    DWORD size;
    DWORD space;
};

struct CHDataFileHeader {
    DWORD id;
    int number;
    unsigned offset;
};

/* datafile */
struct CHDataFile {
    void* m_File;
    CHDataFileIndex* m_Index;
    DWORD m_Id;
    int m_Number;
    CHDataFile() { m_Index = nullptr; m_File = INVALID_HANDLE_VALUE; m_Id = 0; }
};

extern CHDataFile _WDF[MAXDATAFILE];

// Exact same API as original (maintaining function signatures)
CH_CORE_DLL_API void DataFile_Close(CHDataFile* lpDataFile);
CH_CORE_DLL_API BOOL DataFile_IsOpen(CHDataFile* lpDataFile, DWORD id);
CH_CORE_DLL_API BOOL DataFile_IsValid(CHDataFile* lpDataFile);
CH_CORE_DLL_API CHDataFileIndex* DataFile_SearchFile(CHDataFile* lpDataFile, DWORD id);
CH_CORE_DLL_API HANDLE DataFile_GetFileHandle(CHDataFile* lpDataFile);
CH_CORE_DLL_API void* DataFile_Load(const char* pszFile, DWORD& dwSize);
CH_CORE_DLL_API DWORD pack_name(const char* filename);
CH_CORE_DLL_API DWORD real_name(const char* filename);

// Additional functions maintaining exact original API
CH_CORE_DLL_API DWORD stringtoid(const char* str);
CH_CORE_DLL_API DWORD string_id(const char* filename);
CH_CORE_DLL_API void* MyDataFileLoad(const char* filename, DWORD& size);
CH_CORE_DLL_API void MyDataFileClose();
CH_CORE_DLL_API BOOL MyDataFileOpen(const char* filename);
CH_CORE_DLL_API BOOL MyDnpFileOpen(const char* filename);

// Forward declaration of modern DnFile class
class CHDnFileManager;
extern CHDnFileManager g_objDnFile;

// Modern C++ DnFile system (CHDnFile.h equivalent)
class CHDnFileManager {
public:
    CHDnFileManager();
    virtual ~CHDnFileManager();

    // Exact same interface as CDnFile
    void BeforeUseDnFile();
    void AfterUseDnFile();
    FILE* GetFPtr(const char* pszFile, unsigned long& usFileSize);
    void* GetMPtr(const char* pszFile, unsigned long& usFileSize);
    void ClearPtr();
    bool CheckDisperseFile(const char* pszFile);
    bool CheckDisperseFile(const unsigned long uFileID);
    void AddDisperseFile(const char* pszFile);
    bool OpenFile(const char* pszFile);
    void CloseFile(const char* pszFile);

private:
    void Destroy();
    void Create();
    unsigned long GenerateID(const char* pszStr);
    void ProcessDir(const char* pszDir);

    struct FileIndexInfo {
        unsigned long uSize;
        unsigned long uOffset;
    };

    struct DnpInfo {
        FILE* fpDnp;
        std::unordered_map<unsigned long, std::unique_ptr<FileIndexInfo>> mapIndex;
    };

    std::unordered_map<unsigned long, std::unique_ptr<DnpInfo>> m_mapDnp;
    std::unordered_map<unsigned long, unsigned char> m_mapDisperseFiles;
    std::unique_ptr<unsigned char[]> m_pBuffer;
    std::unique_ptr<unsigned char[]> m_pExtendBuffer;
    FILE* m_fpExtend;
    std::mutex m_mutex;

    static const size_t DAWNFILE_BUFFERSIZE = 1024 * 1024; // 1MB default buffer
};

// Compatibility typedefs
typedef CHDataFile C3DataFile;
typedef CHDataFileIndex C3DataFileIndex;
typedef CHDataFileHeader C3DataFileHeader;

#endif // _CH_datafile_h_