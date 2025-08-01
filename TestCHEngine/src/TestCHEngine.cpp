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

// Global variables for camera control
float g_CameraDistance = 10.0f;
float g_CameraAngleX = 0.0f;
float g_CameraAngleY = 0.0f;
bool g_SceneLoaded = false;
bool g_ShowInfo = true;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        case VK_F1:
            g_ShowInfo = !g_ShowInfo;
            return 0;
        case VK_UP:
            g_CameraAngleX += 0.1f;
            return 0;
        case VK_DOWN:
            g_CameraAngleX -= 0.1f;
            return 0;
        case VK_LEFT:
            g_CameraAngleY -= 0.1f;
            return 0;
        case VK_RIGHT:
            g_CameraAngleY += 0.1f;
            return 0;
        case VK_ADD:
        case VK_OEM_PLUS:
            g_CameraDistance -= 1.0f;
            if (g_CameraDistance < 1.0f) g_CameraDistance = 1.0f;
            return 0;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            g_CameraDistance += 1.0f;
            if (g_CameraDistance > 100.0f) g_CameraDistance = 100.0f;
            return 0;
        case 'R':
        case 'r':
            // Reset camera
            g_CameraDistance = 10.0f;
            g_CameraAngleX = 0.0f;
            g_CameraAngleY = 0.0f;
            return 0;
        }
        return 0;
    case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0)
                g_CameraDistance -= 1.0f;
            else
                g_CameraDistance += 1.0f;
            if (g_CameraDistance < 1.0f) g_CameraDistance = 1.0f;
            if (g_CameraDistance > 100.0f) g_CameraDistance = 100.0f;
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize the CH Engine
    int result = Init3D(hInstance, "CH Engine Scene Test", 1280, 720, TRUE, WindowProc, 2);

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

    // Try to load the test scene with detailed error reporting
    printf("Attempting to load test.scene...\n");
    if (Scene_Load(&testScene, "test.scene", 0))
    {
        g_SceneLoaded = true;
        printf("✓ Scene loaded successfully!\n");
        
        // Try to get scene information if available
        if (testScene)
        {
            printf("Scene pointer: %p\n", testScene);
            // Note: Scene structure details would depend on CHScene definition
        }
    }
    else
    {
        g_SceneLoaded = false;
        printf("✗ Scene loading failed!\n");
        testScene = nullptr;
    }

    // Create a test camera with better positioning
    if (!Camera_Load(&testCamera, "test.cam", 0))
    {
        // Create default camera if loading fails
        testCamera = new CHCamera;
        Camera_Clear(testCamera);
        testCamera->fNear = 0.1f;
        testCamera->fFar = 1000.0f;
        testCamera->fFov = CHCameraMath::ToRadian(60.0f);
        
        // Set initial camera position using the animation arrays
        if (testCamera->lpFrom && testCamera->lpTo)
        {
            // Set initial position
            testCamera->lpFrom[0] = XMVectorSet(0.0f, 2.0f, g_CameraDistance, 1.0f);
            testCamera->lpTo[0] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        }
        
        printf("✓ Created default camera\n");
    }
    else
    {
        printf("✓ Camera loaded from test.cam\n");
    }

    // Main loop
    MSG msg = {};
    DWORD frameCount = 0;
    DWORD lastTime = GetTickCount64();
    DWORD lastFPSUpdate = 0;
    DWORD currentFPS = 0;

    printf("Engine test started. Controls:\n");
    printf("- Arrow keys: Rotate camera\n");
    printf("- +/- keys: Zoom in/out\n");
    printf("- Mouse wheel: Zoom in/out\n");
    printf("- R key: Reset camera\n");
    printf("- F1 key: Toggle info display\n");
    printf("- ESC key: Exit\n");

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
                // Update camera position based on controls
                if (testCamera && testCamera->lpFrom && testCamera->lpTo)
                {
                    // Calculate camera position based on spherical coordinates
                    float x = g_CameraDistance * sinf(g_CameraAngleY) * cosf(g_CameraAngleX);
                    float y = g_CameraDistance * sinf(g_CameraAngleX);
                    float z = g_CameraDistance * cosf(g_CameraAngleY) * cosf(g_CameraAngleX);
                    
                    // Update camera position
                    testCamera->lpFrom[0] = XMVectorSet(x, y + 2.0f, z, 1.0f);
                    testCamera->lpTo[0] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
                }

                // Begin rendering
                if (Begin3D())
                {
                    // Clear the screen
                    ClearBuffer(TRUE, TRUE, 0xFF000020); // Dark blue background

                    // Build camera matrices
                    if (testCamera)
                    {
                        Camera_BuildView(testCamera, TRUE);
                        Camera_BuildProject(testCamera, TRUE);
                    }

                    // Draw 3D scene if available
                    if (testScene && g_SceneLoaded)
                    {
                        Scene_Prepare();
                        Scene_Draw(testScene);
                        Scene_NextFrame(testScene, 1);
                    }

                    // Draw 2D sprite if available (as overlay)
                    if (testSprite)
                    {
                        Sprite_Prepare();
                        Sprite_Draw(testSprite, 0); // Normal blending
                    }

                    // Calculate FPS
                    DWORD currentTime = GetTickCount64();
                    frameCount++;
                    if (currentTime - lastFPSUpdate >= 1000) // Update every second
                    {
                        currentFPS = frameCount;
                        frameCount = 0;
                        lastFPSUpdate = currentTime;
                    }

                    // Update window title with detailed information
                    char titleText[256];
                    sprintf_s(titleText, "CH Engine Scene Test - FPS: %d | Scene: %s | Distance: %.1f", 
                             currentFPS, 
                             g_SceneLoaded ? "Loaded" : "Failed", 
                             g_CameraDistance);
                    SetWindowTextA(g_hWnd, titleText);

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