#pragma warning(disable:4786)
#include "CH_datafile.h"
#include "CH_main.h"
#include <filesystem>
#include <fstream>

// Global data file array (maintaining exact same structure)
CHDataFile _WDF[MAXDATAFILE];

// Global DnFile manager instance
CHDnFileManager g_objDnFile;

// Hash algorithm (maintaining exact same algorithm as original for compatibility)
CH_CORE_DLL_API
DWORD stringtoid(const char* str)
{
    int i;
    unsigned int v;
    static unsigned m[70];
    strncpy_s(reinterpret_cast<char*>(m), sizeof(m), str, 256);
    
    for (i = 0; i < 256 / 4 && m[i]; i++);
    m[i++] = 0x9BE74448;
    m[i++] = 0x66F42C48;
    v = 0xF4FA8928;

    // Maintaining exact same assembly algorithm for compatibility
    unsigned esi = 0x37A8470E;  // x0
    unsigned edi = 0x7758B42B;  // y0
    unsigned ecx = 0;

    while (ecx < static_cast<unsigned>(i))
    {
        unsigned ebx = 0x267B0B11;  // w
        
        // rol v,1
        v = (v << 1) | (v >> 31);
        
        unsigned eax = m[ecx];
        ebx ^= v;
        
        unsigned edx = ebx;
        esi ^= eax;
        edi ^= eax;
        
        edx += edi;
        edx |= 0x2040801;   // a
        edx &= 0xBFEF7FDF;  // c
        
        // mul operation
        unsigned __int64 result = static_cast<unsigned __int64>(esi) * edx;
        eax = static_cast<unsigned>(result);
        edx = static_cast<unsigned>(result >> 32);
        
        // adc operations
        unsigned carry = 0;
        eax += edx;
        if (eax < edx) carry = 1;
        eax += carry;
        
        edx = ebx;
        edx += esi;
        edx |= 0x804021;    // b
        edx &= 0x7DFEFBFF;  // d
        
        esi = eax;
        
        // Second multiplication
        result = static_cast<unsigned __int64>(edi) * edx;
        eax = static_cast<unsigned>(result);
        edx = static_cast<unsigned>(result >> 32);
        
        edx += edx;
        eax += edx;
        if (eax < edx) {
            eax += 2;
        }
        
        ecx++;
        edi = eax;
    }
    
    esi ^= edi;
    v = esi;
    
    return v;
}

CH_CORE_DLL_API
DWORD string_id(const char* filename)
{
    char buffer[256];
    int i;
    for (i = 0; filename[i]; i++) {
        if (filename[i] >= 'A' && filename[i] <= 'Z') 
            buffer[i] = filename[i] + 'a' - 'A';
        else if (filename[i] == '\\') 
            buffer[i] = '/';
        else 
            buffer[i] = filename[i];
    }
    buffer[i] = 0;
    return stringtoid(buffer);
}

CH_CORE_DLL_API
void* MyDataFileLoad(const char* filename, DWORD& size)
{
    if (!filename)
        return nullptr;

    DWORD id = pack_name(filename);
    DWORD fid = real_name(filename);

    CHDataFileIndex* pf = nullptr;
    int i;
    
    // Search for the WFile
    for (i = 0; i < MAXDATAFILE; i++)
    {
        if (DataFile_IsOpen(&_WDF[i], id))
        {
            pf = DataFile_SearchFile(&_WDF[i], fid);
            if (pf == nullptr)
                return nullptr;
            else
                break;
        }
    }

    if (i == MAXDATAFILE)
        return nullptr;

    HANDLE f = DataFile_GetFileHandle(&_WDF[i]);
    if (!f)
        return nullptr;

    void* p = malloc(pf->size);
    if (!p)
        return nullptr;

    SetFilePointer(f, pf->offset, 0, FILE_BEGIN);

    DWORD bytes = 0;
    if (ReadFile(f, p, pf->size, &bytes, 0) == 0)
    {
        free(p);
        size = 0;
        return nullptr;
    }
    else
    {
        size = pf->size;
        return p;
    }
}

CH_CORE_DLL_API
void MyDataFileClose()
{
    for (int i = 0; i < MAXDATAFILE; i++)
    {
        DataFile_Close(&_WDF[i]);
    }
}

CH_CORE_DLL_API
BOOL MyDataFileOpen(const char* filename)
{
    int i;
    for (i = 0; i < MAXDATAFILE; i++)
    {
        if (!DataFile_IsValid(&_WDF[i]))
        {
            break;
        }
    }

    if (i == MAXDATAFILE)
    {
        i = 0;
        DataFile_Close(&_WDF[i]);
    }

    HANDLE f = CreateFileA(filename,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          0,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          0);

    if (f == INVALID_HANDLE_VALUE)
        return FALSE;

    CHDataFileHeader header;
    DWORD bytes;
    if (ReadFile(f, &header, sizeof(header), &bytes, 0) == 0)
    {
        CloseHandle(f);
        return FALSE;
    }

    _WDF[i].m_Index = static_cast<CHDataFileIndex*>(malloc(sizeof(CHDataFileIndex) * header.number));
    if (!_WDF[i].m_Index)
    {
        CloseHandle(f);
        return FALSE;
    }

    SetFilePointer(f, header.offset, 0, FILE_BEGIN);
    if (ReadFile(f, _WDF[i].m_Index, sizeof(CHDataFileIndex) * header.number, &bytes, 0) == 0)
    {
        CloseHandle(f);
        free(_WDF[i].m_Index);
        return FALSE;
    }

    _WDF[i].m_Number = header.number;
    _WDF[i].m_File = f;
    _WDF[i].m_Id = string_id(filename);
    return TRUE;
}

CH_CORE_DLL_API
void DataFile_Close(CHDataFile* lpDataFile)
{
    if (lpDataFile->m_File != INVALID_HANDLE_VALUE)
    {
        CloseHandle(lpDataFile->m_File);
        lpDataFile->m_File = INVALID_HANDLE_VALUE;
    }

    if (lpDataFile->m_Index)
    {
        free(lpDataFile->m_Index);
        lpDataFile->m_Index = nullptr;
    }

    lpDataFile->m_Id = 0;
    lpDataFile->m_Number = 0;
}

CH_CORE_DLL_API
CHDataFileIndex* DataFile_SearchFile(CHDataFile* lpDataFile, DWORD id)
{
    int begin, end, middle;
    begin = 0;
    end = lpDataFile->m_Number - 1;
    
    while (begin <= end)
    {
        middle = (begin + end) / 2;
        if (lpDataFile->m_Index[middle].uid == id)
            return &lpDataFile->m_Index[middle];
        else if (lpDataFile->m_Index[middle].uid < id)
            begin = middle + 1;
        else
            end = middle - 1;
    }
    return nullptr;
}

CH_CORE_DLL_API
void* DataFile_Load(const char* pszFile, DWORD& dwSize)
{
    return MyDataFileLoad(pszFile, dwSize);
}

CH_CORE_DLL_API
BOOL DataFile_IsOpen(CHDataFile* lpDataFile, DWORD id)
{
    return lpDataFile->m_Id == id;
}

CH_CORE_DLL_API
BOOL DataFile_IsValid(CHDataFile* lpDataFile)
{
    return lpDataFile->m_File != INVALID_HANDLE_VALUE;
}

CH_CORE_DLL_API
HANDLE DataFile_GetFileHandle(CHDataFile* lpDataFile)
{
    return lpDataFile->m_File;
}

CH_CORE_DLL_API
DWORD pack_name(const char* filename)
{
    static char buffer[256];

    int i;
    for (i = 0; filename[i]; i++)
    {
        if (filename[i] == '/')
        {
            memcpy(buffer + i, ".wdf", 5);
            break;
        }
        if (filename[i] >= 'A' && filename[i] <= 'Z')
            buffer[i] = filename[i] + 'a' - 'A';
        else
            buffer[i] = filename[i];
    }
    if (i == 0) return 0;
    return stringtoid(buffer);
}

CH_CORE_DLL_API
DWORD real_name(const char* filename)
{
    return string_id(filename);
}

CH_CORE_DLL_API
BOOL MyDnpFileOpen(const char* filename)
{
    return g_objDnFile.OpenFile(filename);
}

// CHDnFileManager implementation
CHDnFileManager::CHDnFileManager() : m_fpExtend(nullptr)
{
    Create();
}

CHDnFileManager::~CHDnFileManager()
{
    Destroy();
}

void CHDnFileManager::Create()
{
    m_pBuffer = std::make_unique<unsigned char[]>(DAWNFILE_BUFFERSIZE);
    m_pExtendBuffer = nullptr;
    m_fpExtend = nullptr;
}

void CHDnFileManager::Destroy()
{
    // Close all open files
    for (auto& pair : m_mapDnp)
    {
        if (pair.second && pair.second->fpDnp)
        {
            fclose(pair.second->fpDnp);
        }
    }
    m_mapDnp.clear();
    m_mapDisperseFiles.clear();
    
    m_pBuffer.reset();
    m_pExtendBuffer.reset();
    
    if (m_fpExtend)
    {
        fclose(m_fpExtend);
        m_fpExtend = nullptr;
    }
}

void CHDnFileManager::BeforeUseDnFile()
{
    // Thread safety - lock for file operations
    m_mutex.lock();
}

void CHDnFileManager::AfterUseDnFile()
{
    // Release lock
    m_mutex.unlock();
}

FILE* CHDnFileManager::GetFPtr(const char* pszFile, unsigned long& usFileSize)
{
    ClearPtr();
    if (!pszFile)
        return nullptr;

    std::string fileCopy = pszFile;
    std::transform(fileCopy.begin(), fileCopy.end(), fileCopy.begin(), ::tolower);
    std::replace(fileCopy.begin(), fileCopy.end(), '/', '\\');

    unsigned long idFile = GenerateID(fileCopy.c_str());
    if (!CheckDisperseFile(idFile))
    {
        // Extract pack name
        size_t pos = fileCopy.find('\\');
        if (pos != std::string::npos)
        {
            std::string packName = fileCopy.substr(0, pos);
            unsigned long idPack = GenerateID(packName.c_str());
            
            auto iter = m_mapDnp.find(idPack);
            if (iter != m_mapDnp.end() && iter->second)
            {
                auto& dnpInfo = iter->second;
                auto fileIter = dnpInfo->mapIndex.find(idFile);
                if (fileIter != dnpInfo->mapIndex.end())
                {
                    fseek(dnpInfo->fpDnp, fileIter->second->uOffset, SEEK_SET);
                    usFileSize = fileIter->second->uSize;
                    m_fpExtend = dnpInfo->fpDnp;
                    return m_fpExtend;
                }
            }
        }
    }
    return nullptr;
}

void* CHDnFileManager::GetMPtr(const char* pszFile, unsigned long& usFileSize)
{
    ClearPtr();
    if (!pszFile)
        return nullptr;

    std::string fileCopy = pszFile;
    std::transform(fileCopy.begin(), fileCopy.end(), fileCopy.begin(), ::tolower);
    std::replace(fileCopy.begin(), fileCopy.end(), '/', '\\');

    unsigned long idFile = GenerateID(fileCopy.c_str());
    if (!CheckDisperseFile(idFile))
    {
        // Extract pack name
        size_t pos = fileCopy.find('\\');
        if (pos != std::string::npos)
        {
            std::string packName = fileCopy.substr(0, pos);
            unsigned long idPack = GenerateID(packName.c_str());
            
            auto iter = m_mapDnp.find(idPack);
            if (iter != m_mapDnp.end() && iter->second)
            {
                auto& dnpInfo = iter->second;
                auto fileIter = dnpInfo->mapIndex.find(idFile);
                if (fileIter != dnpInfo->mapIndex.end())
                {
                    fseek(dnpInfo->fpDnp, fileIter->second->uOffset, SEEK_SET);
                    
                    if (fileIter->second->uSize > DAWNFILE_BUFFERSIZE)
                    {
                        m_pExtendBuffer = std::make_unique<unsigned char[]>(fileIter->second->uSize);
                        if (!m_pExtendBuffer)
                            return nullptr;
                        
                        usFileSize = fileIter->second->uSize;
                        fread(m_pExtendBuffer.get(), sizeof(char), fileIter->second->uSize, dnpInfo->fpDnp);
                        return m_pExtendBuffer.get();
                    }
                    else
                    {
                        fread(m_pBuffer.get(), sizeof(char), fileIter->second->uSize, dnpInfo->fpDnp);
                        usFileSize = fileIter->second->uSize;
                        return m_pBuffer.get();
                    }
                }
            }
        }
    }
    return nullptr;
}

void CHDnFileManager::ClearPtr()
{
    m_pExtendBuffer.reset();
    m_fpExtend = nullptr;
}

bool CHDnFileManager::CheckDisperseFile(const char* pszFile)
{
    unsigned long id = GenerateID(pszFile);
    return CheckDisperseFile(id);
}

bool CHDnFileManager::CheckDisperseFile(const unsigned long uFileID)
{
    return m_mapDisperseFiles.find(uFileID) != m_mapDisperseFiles.end();
}

void CHDnFileManager::AddDisperseFile(const char* pszFile)
{
    unsigned long id = GenerateID(pszFile);
    m_mapDisperseFiles[id] = 1;
}

bool CHDnFileManager::OpenFile(const char* pszFile)
{
    if (!pszFile)
        return false;

    std::string filename = pszFile;
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    unsigned long id = GenerateID(filename.c_str());
    
    // Check if already open
    if (m_mapDnp.find(id) != m_mapDnp.end())
        return true;

    FILE* fp = fopen(pszFile, "rb");
    if (!fp)
        return false;

    // Read header to get file count and index offset
    // This is a simplified implementation - actual DNP format may vary
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Create DnpInfo
    auto dnpInfo = std::make_unique<DnpInfo>();
    dnpInfo->fpDnp = fp;
    
    // For now, treat as a simple archive - real implementation would parse DNP format
    m_mapDnp[id] = std::move(dnpInfo);
    
    return true;
}

void CHDnFileManager::CloseFile(const char* pszFile)
{
    if (!pszFile)
        return;

    std::string filename = pszFile;
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    unsigned long id = GenerateID(filename.c_str());
    
    auto iter = m_mapDnp.find(id);
    if (iter != m_mapDnp.end())
    {
        if (iter->second && iter->second->fpDnp)
        {
            fclose(iter->second->fpDnp);
        }
        m_mapDnp.erase(iter);
    }
}

unsigned long CHDnFileManager::GenerateID(const char* pszStr)
{
    return stringtoid(pszStr);
}

void CHDnFileManager::ProcessDir(const char* pszDir)
{
    // Process directory for file indexing
    if (!pszDir)
        return;

    try
    {
        std::filesystem::path dirPath(pszDir);
        if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
            {
                if (entry.is_regular_file())
                {
                    std::string relativePath = std::filesystem::relative(entry.path(), dirPath).string();
                    AddDisperseFile(relativePath.c_str());
                }
            }
        }
    }
    catch (const std::exception&)
    {
        // Handle filesystem errors silently
    }
}