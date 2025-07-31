#include "CH_capscreen.h"
#include "CH_main.h"

void CapScreen(char* lpName)
{
    if (!lpName)
        return;
    
    unsigned char* buffer = nullptr;
    DWORD width, height;
    
    // Capture the back buffer
    if (CHCapScreenInternal::CaptureBackBuffer(&buffer, &width, &height))
    {
        // Determine file format from extension
        std::string filename(lpName);
        std::string extension = filename.substr(filename.find_last_of('.') + 1);
        
        // Convert to lowercase for comparison
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "jpg" || extension == "jpeg")
        {
            CHCapScreenInternal::SaveToJPEG(lpName, buffer, width, height, 90); // 90% quality
        }
        else if (extension == "bmp")
        {
            CHCapScreenInternal::SaveToBMP(lpName, buffer, width, height);
        }
        else
        {
            // Default to JPEG
            CHCapScreenInternal::SaveToJPEG(lpName, buffer, width, height, 90);
        }
        
        delete[] buffer;
    }
}

BOOL JPGEncode(char* lpJpegName, DWORD dwWidth, DWORD dwHeight, 
               unsigned char* lpBuffer, int mode, DWORD dwQuality)
{
    if (!lpJpegName || !lpBuffer || dwWidth == 0 || dwHeight == 0)
        return FALSE;
    
    // Convert BGR to RGB if needed
    if (mode == 1) // BGR mode
    {
        CHCapScreenInternal::ConvertRGBtoBGR(lpBuffer, dwWidth, dwHeight);
    }
    
    return CHCapScreenInternal::SaveToJPEG(lpJpegName, lpBuffer, dwWidth, dwHeight, dwQuality);
}

// Internal implementation
namespace CHCapScreenInternal {

BOOL CaptureBackBuffer(unsigned char** buffer, DWORD* width, DWORD* height)
{
    if (!buffer || !width || !height || !g_SwapChain)
        return FALSE;
    
    // Get the back buffer
    CHComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), 
                                       reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    if (FAILED(hr))
        return FALSE;
    
    // Get back buffer description
    D3D11_TEXTURE2D_DESC backBufferDesc;
    backBuffer->GetDesc(&backBufferDesc);
    
    *width = backBufferDesc.Width;
    *height = backBufferDesc.Height;
    
    // Create staging texture for CPU access
    D3D11_TEXTURE2D_DESC stagingDesc = backBufferDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;
    
    CHComPtr<ID3D11Texture2D> stagingTexture;
    hr = g_D3DDevice->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr))
        return FALSE;
    
    // Copy back buffer to staging texture
    g_D3DContext->CopyResource(stagingTexture.Get(), backBuffer.Get());
    
    // Map staging texture for reading
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = g_D3DContext->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr))
        return FALSE;
    
    // Allocate buffer for image data
    DWORD imageSize = *width * *height * 4; // RGBA
    *buffer = new unsigned char[imageSize];
    
    // Copy pixel data
    unsigned char* src = static_cast<unsigned char*>(mappedResource.pData);
    unsigned char* dst = *buffer;
    
    for (DWORD y = 0; y < *height; y++)
    {
        memcpy(dst + (y * *width * 4), src + (y * mappedResource.RowPitch), *width * 4);
    }
    
    // Unmap the staging texture
    g_D3DContext->Unmap(stagingTexture.Get(), 0);
    
    // Flip image vertically (DirectX textures are stored top-down, but images are usually bottom-up)
    FlipImageVertically(*buffer, *width, *height, 4);
    
    return TRUE;
}

BOOL SaveToJPEG(const char* filename, unsigned char* buffer, DWORD width, DWORD height, DWORD quality)
{
    if (!filename || !buffer)
        return FALSE;
    
    // This is a simplified implementation
    // In a full implementation, you would use a library like libjpeg or Windows Imaging Component (WIC)
    
    // For now, we'll save as BMP since JPEG encoding requires additional libraries
    return SaveToBMP(filename, buffer, width, height);
}

BOOL SaveToBMP(const char* filename, unsigned char* buffer, DWORD width, DWORD height)
{
    if (!filename || !buffer)
        return FALSE;
    
    FILE* file = fopen(filename, "wb");
    if (!file)
        return FALSE;
    
    // BMP file header
    BITMAPFILEHEADER fileHeader = {};
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (width * height * 3);
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    // BMP info header
    BITMAPINFOHEADER infoHeader = {};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = static_cast<LONG>(width);
    infoHeader.biHeight = static_cast<LONG>(height);
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24; // RGB
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = width * height * 3;
    
    // Write headers
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);
    
    // Convert RGBA to RGB and write pixel data
    for (DWORD y = 0; y < height; y++)
    {
        for (DWORD x = 0; x < width; x++)
        {
            DWORD srcIndex = ((height - 1 - y) * width + x) * 4; // Flip Y coordinate
            
            // Write BGR (BMP format)
            fputc(buffer[srcIndex + 2], file); // B
            fputc(buffer[srcIndex + 1], file); // G
            fputc(buffer[srcIndex + 0], file); // R
        }
        
        // BMP rows must be aligned to 4-byte boundaries
        int padding = (4 - ((width * 3) % 4)) % 4;
        for (int p = 0; p < padding; p++)
        {
            fputc(0, file);
        }
    }
    
    fclose(file);
    return TRUE;
}

void ConvertRGBtoBGR(unsigned char* buffer, DWORD width, DWORD height)
{
    if (!buffer)
        return;
    
    for (DWORD i = 0; i < width * height; i++)
    {
        DWORD index = i * 4; // Assuming RGBA format
        
        // Swap R and B channels
        unsigned char temp = buffer[index];     // R
        buffer[index] = buffer[index + 2];      // R = B
        buffer[index + 2] = temp;               // B = R
        // G and A remain unchanged
    }
}

void FlipImageVertically(unsigned char* buffer, DWORD width, DWORD height, DWORD bytesPerPixel)
{
    if (!buffer)
        return;
    
    DWORD rowSize = width * bytesPerPixel;
    unsigned char* tempRow = new unsigned char[rowSize];
    
    for (DWORD y = 0; y < height / 2; y++)
    {
        DWORD topRowOffset = y * rowSize;
        DWORD bottomRowOffset = (height - 1 - y) * rowSize;
        
        // Swap rows
        memcpy(tempRow, buffer + topRowOffset, rowSize);
        memcpy(buffer + topRowOffset, buffer + bottomRowOffset, rowSize);
        memcpy(buffer + bottomRowOffset, tempRow, rowSize);
    }
    
    delete[] tempRow;
}

} // namespace CHCapScreenInternal