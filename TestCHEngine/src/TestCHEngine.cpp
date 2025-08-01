// Test program for CH Engine
// File: TestCHEngine.cpp

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <winuser.h>
#include "CH_main.h"
#include "CH_sprite.h"
#include "CH_scene.h"
#include "CH_camera.h"
#include "CH_texture.h"
#include <CH_datafile.h>

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize the CH Engine
    int result = Init3D(hInstance, "CH Engine Test", 1024, 768, TRUE, WindowProc, 2);

    if (result <= 0)
    {
        MessageBoxA(NULL, "Failed to initialize CH Engine!", "Error", MB_OK);
        return -1;
    }

    // Test basic functionality
    CHSprite* testSprite = nullptr;
    CHScene* testScene = nullptr;
    CHCamera* testCamera = nullptr;

    // Create a test sprite (will use default texture if file doesn't exist)
    if (Sprite_Load(&testSprite, "test.bmp", CH_POOL_MANAGED, TRUE, 0))
    {
        // Set sprite position
        Sprite_SetCoor(testSprite, nullptr, 100, 100, 0, 0);
        Sprite_SetColor(testSprite, 255, 255, 255, 255);
    }

    // Try to load a test scene
    if (!Scene_Load(&testScene, "test.scene", 0))
    {
        // Scene loading failed, that's OK for testing
        testScene = nullptr;
    }

    // Create a test camera
    if (!Camera_Load(&testCamera, "test.cam", 0))
    {
        // Create default camera if loading fails
        testCamera = new CHCamera;
        Camera_Clear(testCamera);
        testCamera->fNear = 1.0f;
        testCamera->fFar = 1000.0f;
        testCamera->fFov = CHCameraMath::ToRadian(45.0f);
    }

    // Main loop
    MSG msg = {};
    DWORD frameCount = 0;
    DWORD lastTime = GetTickCount64();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Limit frame rate
            if (LimitRate(16)) // ~60 FPS
            {
                // Begin rendering
                if (Begin3D())
                {
                    // Clear the screen
                    ClearBuffer(TRUE, TRUE, 0xFF000040); // Dark blue background

                    // Build camera matrices
                    if (testCamera)
                    {
                        Camera_BuildView(testCamera, TRUE);
                        Camera_BuildProject(testCamera, TRUE);
                    }

                    // Draw 3D scene if available
                    if (testScene)
                    {
                        Scene_Prepare();
                        Scene_Draw(testScene);
                        Scene_NextFrame(testScene, 1);
                    }

                    // Draw 2D sprite if available
                    if (testSprite)
                    {
                        Sprite_Prepare();
                        Sprite_Draw(testSprite, 0); // Normal blending
                    }

                    // Draw frame rate
                    DWORD currentRate = CalcRate();
                    char rateText[64];
                    sprintf_s(rateText, "FPS: %d", currentRate);

                    // You could draw text here if font system is implemented
                    // For now, just update window title periodically
                    frameCount++;
                    if (frameCount % 60 == 0) // Update every 60 frames
                    {
                        char titleText[128];
                        sprintf_s(titleText, "CH Engine Test - FPS: %d", currentRate);
                        SetWindowTextA(g_hWnd, titleText);
                    }

                    // End rendering
                    End3D();

                    // Present the frame
                    Flip();
                }
            }

            // Check for device lost (compatibility)
            if (IfDeviceLost())
            {
                ResetDevice();
            }
        }
    }

    // Cleanup
    if (testSprite)
        Sprite_Unload(&testSprite);

    if (testScene)
        Scene_Unload(&testScene);

    if (testCamera)
        Camera_Unload(&testCamera);

    // Shutdown the engine
    Quit3D();

    return static_cast<int>(msg.wParam);
}

// Alternative console test for debugging
void ConsoleTest()
{
    printf("CH Engine Console Test\n");
    printf("======================\n\n");

    // Test texture loading
    printf("Testing texture system...\n");
    CHTexture* testTex;
    int texID = Texture_Load(&testTex, "test.bmp", 1, CH_POOL_MANAGED, TRUE, 0);
    if (texID >= 0)
    {
        printf("✓ Texture loaded successfully (ID: %d)\n", texID);
        printf("  Size: %dx%d\n", testTex->Info.Width, testTex->Info.Height);
        printf("  Format: %d\n", testTex->Info.Format);
        Texture_Unload(&testTex);
    }
    else
    {
        printf("✗ Texture loading failed (expected for test)\n");
    }

    // Test data file system
    printf("\nTesting data file system...\n");
    if (MyDataFileOpen("test.wdf"))
    {
        printf("✓ Data file opened successfully\n");
        MyDataFileClose();
    }
    else
    {
        printf("✗ Data file opening failed (expected for test)\n");
    }

    // Test math utilities
    printf("\nTesting math utilities...\n");
    float radians = CHCameraMath::ToRadian(90.0f);
    float degrees = CHCameraMath::ToDegree(radians);
    printf("✓ 90 degrees = %.4f radians = %.4f degrees\n", radians, degrees);

    // Test random number generation
    printf("\nTesting random numbers...\n");
    for (int i = 0; i < 5; i++)
    {
        int rand = Random(1, 100);
        printf("✓ Random number %d: %d\n", i + 1, rand);
    }

    printf("\nConsole test complete!\n");
}

// Simple bitmap creation for testing (24-bit BMP)
bool CreateTestBitmap(const char* filename, int width, int height)
{
    FILE* file = fopen(filename, "wb");
    if (!file)
        return false;

    // BMP file header
    BITMAPFILEHEADER fileHeader = {};
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (width * height * 3);
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // BMP info header
    BITMAPINFOHEADER infoHeader = {};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = width * height * 3;

    // Write headers
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    // Create a simple gradient pattern
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            unsigned char r = (unsigned char)((x * 255) / width);
            unsigned char g = (unsigned char)((y * 255) / height);
            unsigned char b = 128;

            // Write BGR (BMP format)
            fputc(b, file);
            fputc(g, file);
            fputc(r, file);
        }

        // BMP rows must be aligned to 4-byte boundaries
        int padding = (4 - ((width * 3) % 4)) % 4;
        for (int p = 0; p < padding; p++)
        {
            fputc(0, file);
        }
    }

    fclose(file);
    return true;
}

// Entry point that creates test files and runs engine
int main()
{
    printf("Creating test files...\n");

    // Create a test bitmap
    if (CreateTestBitmap("test.bmp", 256, 256))
    {
        printf("✓ Created test.bmp\n");
    }

    // Run console test first
    ConsoleTest();

    printf("\nPress Enter to run graphics test (or Ctrl+C to exit)...");
    getchar();

    // Run the graphics test
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}