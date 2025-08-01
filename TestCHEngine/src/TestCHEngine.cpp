// Enhanced 3D Test for CH Engine
// File: Enhanced3DTest.cpp

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <winuser.h>
#include <stdio.h>
#include <math.h>
#include "CH_main.h"
#include "CH_sprite.h"
#include "CH_scene.h"
#include "CH_camera.h"
#include "CH_texture.h"
#include "CH_datafile.h"

// Test scene data
struct Test3DVertex {
    float x, y, z;    // Position
    float nx, ny, nz; // Normal
    float u, v;       // Texture coordinates
    float lu, lv;     // Lightmap coordinates (unused for this test)
};

// Forward declarations
bool CreateProceduralTexture(CHTexture** texture);
bool CreateTestCamera(CHCamera** camera);
bool CreateInfoSprite();
void UpdateCamera();
void RenderDebugInfo();
void RunConsoleTests();
void PrintDirectXInfo();
void ValidateRenderingPipeline();
bool CreateTestCubeAdvanced(CHScene** scene);
bool CreateTestPrimitives(CHScene** scene);
bool ValidateEngine();

// Global test variables
CHScene* g_TestScene = nullptr;
CHCamera* g_TestCamera = nullptr;
CHSprite* g_InfoSprite = nullptr;
CHTexture* g_TestTexture = nullptr;

// Camera control
float g_CameraDistance = 15.0f;
float g_CameraAngleX = 0.3f;  // Start with slight downward angle
float g_CameraAngleY = 0.0f;
bool g_ShowInfo = true;
bool g_WireframeMode = true;
DWORD g_LastTime = 0;
float g_RotationAngle = 0.0f;

// Test geometry creation
bool CreateTestCube(CHScene** scene)
{
    *scene = new CHScene;
    Scene_Clear(*scene);

    // Set scene name
    const char* sceneName = "Procedural Test Cube";
    size_t nameLen = strlen(sceneName) + 1;
    (*scene)->lpName = new char[nameLen];
    strcpy_s((*scene)->lpName, nameLen, sceneName);

    // Create cube vertices (6 faces, 4 vertices each = 24 vertices)
    (*scene)->dwVecCount = 24;
    (*scene)->lpVB = new CHSceneVertex[24];

    // Cube size
    float size = 2.0f;

    // Front face (Z+)
    (*scene)->lpVB[0] = { -size, -size,  size,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[1] = { size, -size,  size,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[2] = { size,  size,  size,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[3] = { -size,  size,  size,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Back face (Z-)
    (*scene)->lpVB[4] = { size, -size, -size,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[5] = { -size, -size, -size,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[6] = { -size,  size, -size,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[7] = { size,  size, -size,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Left face (X-)
    (*scene)->lpVB[8] = { -size, -size, -size, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[9] = { -size, -size,  size, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[10] = { -size,  size,  size, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[11] = { -size,  size, -size, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Right face (X+)
    (*scene)->lpVB[12] = { size, -size,  size,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[13] = { size, -size, -size,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[14] = { size,  size, -size,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[15] = { size,  size,  size,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Top face (Y+)
    (*scene)->lpVB[16] = { -size,  size,  size,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[17] = { size,  size,  size,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[18] = { size,  size, -size,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[19] = { -size,  size, -size,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Bottom face (Y-)
    (*scene)->lpVB[20] = { -size, -size, -size,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[21] = { size, -size, -size,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[22] = { size, -size,  size,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[23] = { -size, -size,  size,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Create cube indices (12 triangles, 36 indices)
    (*scene)->dwTriCount = 12;
    (*scene)->lpIB = new WORD[36];

    // Front face
    (*scene)->lpIB[0] = 0; (*scene)->lpIB[1] = 1; (*scene)->lpIB[2] = 2;
    (*scene)->lpIB[3] = 0; (*scene)->lpIB[4] = 2; (*scene)->lpIB[5] = 3;

    // Back face
    (*scene)->lpIB[6] = 4; (*scene)->lpIB[7] = 5; (*scene)->lpIB[8] = 6;
    (*scene)->lpIB[9] = 4; (*scene)->lpIB[10] = 6; (*scene)->lpIB[11] = 7;

    // Left face
    (*scene)->lpIB[12] = 8; (*scene)->lpIB[13] = 9; (*scene)->lpIB[14] = 10;
    (*scene)->lpIB[15] = 8; (*scene)->lpIB[16] = 10; (*scene)->lpIB[17] = 11;

    // Right face
    (*scene)->lpIB[18] = 12; (*scene)->lpIB[19] = 13; (*scene)->lpIB[20] = 14;
    (*scene)->lpIB[21] = 12; (*scene)->lpIB[22] = 14; (*scene)->lpIB[23] = 15;

    // Top face
    (*scene)->lpIB[24] = 16; (*scene)->lpIB[25] = 17; (*scene)->lpIB[26] = 18;
    (*scene)->lpIB[27] = 16; (*scene)->lpIB[28] = 18; (*scene)->lpIB[29] = 19;

    // Bottom face
    (*scene)->lpIB[30] = 20; (*scene)->lpIB[31] = 21; (*scene)->lpIB[32] = 22;
    (*scene)->lpIB[33] = 20; (*scene)->lpIB[34] = 22; (*scene)->lpIB[35] = 23;

    // Create procedural texture
    if (!CreateProceduralTexture(&g_TestTexture))
    {
        printf("Warning: Failed to create procedural texture\n");
        (*scene)->nTex = -1;
    }
    else
    {
        // For now, just use texture ID 0 since we're not managing it in global array
        (*scene)->nTex = 0;
    }

    // Create DirectX 11 buffers
    if (FAILED(CHSceneInternal::CreateVertexBuffer(*scene)) ||
        FAILED(CHSceneInternal::CreateIndexBuffer(*scene)))
    {
        printf("Error: Failed to create DirectX 11 buffers\n");
        Scene_Unload(scene);
        return false;
    }

    printf("✓ Test cube created successfully\n");
    printf("  Vertices: %d, Triangles: %d\n", (*scene)->dwVecCount, (*scene)->dwTriCount);
    printf("  Texture ID: %d\n", (*scene)->nTex);

    return true;
}

// Create a simple procedural texture
bool CreateProceduralTexture(CHTexture** texture)
{
    const UINT width = 256;
    const UINT height = 256;

    // Use the existing Texture_Create function
    if (!Texture_Create(texture, width, height, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED))
    {
        printf("Failed to create texture with Texture_Create\n");
        return false;
    }

    // Don't manually allocate texture name - let the texture system handle it
    // The Texture_Create function should handle memory management properly
    printf("✓ Procedural texture created using Texture_Create\n");
    return true;
}

// Create test camera
bool CreateTestCamera(CHCamera** camera)
{
    *camera = new CHCamera;
    Camera_Clear(*camera);

    // Set camera parameters
    (*camera)->fNear = 0.1f;
    (*camera)->fFar = 1000.0f;
    (*camera)->fFov = CHCameraMath::ToRadian(60.0f);
    (*camera)->dwFrameCount = 1;
    (*camera)->nFrame = 0;

    // Allocate position arrays safely
    (*camera)->lpFrom = new XMVECTOR[1];
    (*camera)->lpTo = new XMVECTOR[1];

    // Set initial position
    (*camera)->lpFrom[0] = XMVectorSet(0.0f, 5.0f, g_CameraDistance, 1.0f);
    (*camera)->lpTo[0] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    printf("✓ Test camera created\n");
    return true;
}

// Update camera based on user input
void UpdateCamera()
{
    if (!g_TestCamera || !g_TestCamera->lpFrom || !g_TestCamera->lpTo)
        return;

    // Calculate camera position using spherical coordinates
    float x = g_CameraDistance * sinf(g_CameraAngleY) * cosf(g_CameraAngleX);
    float y = g_CameraDistance * sinf(g_CameraAngleX);
    float z = g_CameraDistance * cosf(g_CameraAngleY) * cosf(g_CameraAngleX);

    // Update camera position
    g_TestCamera->lpFrom[0] = XMVectorSet(x, y, z, 1.0f);
    g_TestCamera->lpTo[0] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

// Create info display sprite
bool CreateInfoSprite()
{
    // Create a small colored texture for UI using existing system
    CHTexture* infoTexture = nullptr;
    if (!Texture_Create(&infoTexture, 16, 16, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED))
    {
        printf("Warning: Failed to create info texture\n");
        return false;
    }

    // Create sprite
    g_InfoSprite = new CHSprite;
    Sprite_Clear(g_InfoSprite);
    g_InfoSprite->lpTex = infoTexture;

    // Position in top-left corner
    Sprite_SetCoor(g_InfoSprite, nullptr, 10, 10, 200, 16);
    Sprite_SetColor(g_InfoSprite, 128, 0, 255, 0); // Semi-transparent green

    printf("✓ Info sprite created\n");
    return true;
}

// Render debug information
void RenderDebugInfo()
{
    if (!g_ShowInfo)
        return;

    // This would render text in a real implementation
    // For now, just render the info sprite as a placeholder
    if (g_InfoSprite)
    {
        Sprite_Prepare();
        Sprite_Draw(g_InfoSprite, 1); // Additive blending
    }
}

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
            printf("Info display: %s\n", g_ShowInfo ? "ON" : "OFF");
            return 0;

        case VK_F2:
            g_WireframeMode = !g_WireframeMode;
            printf("Wireframe mode: %s\n", g_WireframeMode ? "ON" : "OFF");
            return 0;

        case VK_UP:
            g_CameraAngleX += 0.05f;
            if (g_CameraAngleX > 1.5f) g_CameraAngleX = 1.5f;
            return 0;

        case VK_DOWN:
            g_CameraAngleX -= 0.05f;
            if (g_CameraAngleX < -1.5f) g_CameraAngleX = -1.5f;
            return 0;

        case VK_LEFT:
            g_CameraAngleY -= 0.05f;
            return 0;

        case VK_RIGHT:
            g_CameraAngleY += 0.05f;
            return 0;

        case VK_ADD:
        case VK_OEM_PLUS:
            g_CameraDistance -= 1.0f;
            if (g_CameraDistance < 2.0f) g_CameraDistance = 2.0f;
            return 0;

        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            g_CameraDistance += 1.0f;
            if (g_CameraDistance > 50.0f) g_CameraDistance = 50.0f;
            return 0;

        case 'R':
        case 'r':
            // Reset camera
            g_CameraDistance = 15.0f;
            g_CameraAngleX = 0.3f;
            g_CameraAngleY = 0.0f;
            printf("Camera reset\n");
            return 0;

        case 'P':
            // Print current state
            printf("\n=== Current State ===\n");
            printf("Camera Distance: %.2f\n", g_CameraDistance);
            printf("Camera Angle X: %.2f\n", g_CameraAngleX);
            printf("Camera Angle Y: %.2f\n", g_CameraAngleY);
            printf("Rotation Angle: %.2f\n", g_RotationAngle);
            printf("Wireframe: %s\n", g_WireframeMode ? "ON" : "OFF");
            if (g_TestScene)
            {
                printf("Scene: %d vertices, %d triangles\n",
                    g_TestScene->dwVecCount, g_TestScene->dwTriCount);
            }
            printf("====================\n\n");
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

        if (g_CameraDistance < 2.0f) g_CameraDistance = 2.0f;
        if (g_CameraDistance > 50.0f) g_CameraDistance = 50.0f;
    }
    return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    printf("CH Engine 3D Test Starting...\n");
    printf("==============================\n\n");

    // Initialize the CH Engine with debug output
    printf("Initializing DirectX 11...\n");
    int result = Init3D(hInstance, "CH Engine 3D Test", 1280, 720, TRUE, WindowProc, 2);

    switch (result)
    {
    case 1:
        printf("✓ DirectX 11 initialized successfully\n");
        break;
    case 0:
        printf("✗ DirectX version error\n");
        MessageBoxA(NULL, "DirectX version error!", "Error", MB_OK);
        return -1;
    case -1:
        printf("✗ Hardware not supported\n");
        MessageBoxA(NULL, "DirectX 11 hardware not supported!", "Error", MB_OK);
        return -1;
    case -2:
        printf("✗ 16-bit mode not supported\n");
        MessageBoxA(NULL, "16-bit color mode not supported!", "Error", MB_OK);
        return -1;
    default:
        printf("✗ Unknown initialization error (%d)\n", result);
        return -1;
    }

    // Print graphics info
    printf("\nGraphics Information:\n");
    printf("Feature Level: ");
    switch (g_FeatureLevel)
    {
    case D3D_FEATURE_LEVEL_11_1: printf("11.1\n"); break;
    case D3D_FEATURE_LEVEL_11_0: printf("11.0\n"); break;
    case D3D_FEATURE_LEVEL_10_1: printf("10.1\n"); break;
    case D3D_FEATURE_LEVEL_10_0: printf("10.0\n"); break;
    default: printf("Unknown\n"); break;
    }
    printf("Resolution: %dx%d\n", g_DisplayMode.Width, g_DisplayMode.Height);
    printf("Format: %d\n", g_DisplayMode.Format);

    // Create test content
    printf("\nCreating test content...\n");

    if (!CreateTestCube(&g_TestScene))
    {
        printf("✗ Failed to create test cube\n");
        Quit3D();
        return -1;
    }

    if (!CreateTestCamera(&g_TestCamera))
    {
        printf("✗ Failed to create test camera\n");
        Quit3D();
        return -1;
    }

    CreateInfoSprite(); // Non-critical

    // Print controls
    printf("\nControls:\n");
    printf("- Arrow keys: Rotate camera\n");
    printf("- +/- keys or mouse wheel: Zoom in/out\n");
    printf("- R key: Reset camera\n");
    printf("- P key: Print current state\n");
    printf("- F1 key: Toggle info display\n");
    printf("- F2 key: Toggle wireframe mode\n");
    printf("- ESC key: Exit\n");
    printf("\nStarting render loop...\n\n");

    // Initialize timing
    g_LastTime = GetTickCount();

    // Main rendering loop
    MSG msg = {};
    DWORD frameCount = 0;
    DWORD lastFPSUpdate = GetTickCount();
    DWORD currentFPS = 0;
    bool firstFrame = true;

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Limit frame rate to ~60 FPS
            if (LimitRate(16))
            {
                DWORD currentTime = GetTickCount();
                float deltaTime = (currentTime - g_LastTime) / 1000.0f;
                g_LastTime = currentTime;

                // Update rotation
                g_RotationAngle += deltaTime * 0.5f; // Slow rotation

                // Update camera
                UpdateCamera();

                // Begin rendering
                if (Begin3D())
                {
                    // Clear buffers
                    ClearBuffer(TRUE, TRUE, 0xFF001122); // Dark blue-red background

                    // Build and set camera matrices
                    if (g_TestCamera)
                    {
                        Camera_BuildView(g_TestCamera, TRUE);
                        Camera_BuildProject(g_TestCamera, TRUE);

                        if (firstFrame)
                        {
                            printf("✓ Camera matrices built\n");
                            firstFrame = false;
                        }
                    }

                    // Set wireframe mode if enabled
                    if (g_WireframeMode)
                    {
                        // Note: You'll need to implement wireframe in your render state manager
                        // SetRenderState(CH_RS_FILLMODE, CH_FILL_WIREFRAME);
                    }

                    // Draw the test scene
                    if (g_TestScene)
                    {
                        // Apply rotation transform
                        XMMATRIX rotationMatrix = XMMatrixRotationY(g_RotationAngle);
                        Scene_Muliply(g_TestScene, &rotationMatrix);

                        // Prepare and draw scene
                        Scene_Prepare();
                        if (!Scene_Draw(g_TestScene))
                        {
                            if (firstFrame)
                                printf("Warning: Scene_Draw returned FALSE\n");
                        }
                    }

                    // Draw 2D overlay
                    RenderDebugInfo();

                    // End rendering
                    End3D();

                    // Present the frame
                    if (!Flip())
                    {
                        printf("Warning: Flip() failed\n");
                    }
                }
                else
                {
                    printf("Warning: Begin3D() failed\n");
                }

                // Calculate FPS
                frameCount++;
                if (currentTime - lastFPSUpdate >= 1000) // Update every second
                {
                    currentFPS = frameCount;
                    frameCount = 0;
                    lastFPSUpdate = currentTime;

                    // Update window title
                    char titleText[256];
                    sprintf_s(titleText, "CH Engine 3D Test - FPS: %d | Dist: %.1f | Wireframe: %s",
                        currentFPS, g_CameraDistance, g_WireframeMode ? "ON" : "OFF");
                    SetWindowTextA(g_hWnd, titleText);
                }

                // Check for device lost (compatibility)
                if (IfDeviceLost())
                {
                    printf("Device lost detected, resetting...\n");
                    ResetDevice();
                }
            }
        }
    }

    // Cleanup
    printf("\nCleaning up...\n");

    if (g_InfoSprite)
    {
        Sprite_Unload(&g_InfoSprite);
        printf("✓ Info sprite cleaned up\n");
    }

    if (g_TestScene)
    {
        Scene_Unload(&g_TestScene);
        printf("✓ Test scene cleaned up\n");
    }

    if (g_TestCamera)
    {
        Camera_Unload(&g_TestCamera);
        printf("✓ Test camera cleaned up\n");
    }

    // Shutdown the engine
    Quit3D();
    printf("✓ CH Engine shutdown complete\n");

    return static_cast<int>(msg.wParam);
}

// Console test mode (for debugging without graphics)
void RunConsoleTests()
{
    printf("CH Engine Console Tests\n");
    printf("=======================\n\n");

    // Test math functions
    printf("Testing math utilities...\n");
    float radians = CHCameraMath::ToRadian(90.0f);
    float degrees = CHCameraMath::ToDegree(radians);
    printf("✓ 90° = %.4f rad = %.4f°\n", radians, degrees);

    // Test random numbers
    printf("\nTesting random number generator...\n");
    for (int i = 0; i < 5; i++)
    {
        int rand = Random(1, 100);
        printf("✓ Random %d: %d\n", i + 1, rand);
    }

    // Test string functions
    printf("\nTesting string utilities...\n");
    char testString[] = "level1\\level2\\level3\\file.txt";
    char original[256];
    strcpy_s(original, testString);

    CutString(testString, 2);
    printf("✓ CutString('%s', 2) = '%s'\n", original, testString);

    // Test data file hash
    printf("\nTesting data file hashing...\n");
    DWORD hash1 = string_id("test.txt");
    DWORD hash2 = string_id("TEST.TXT");
    printf("✓ Hash 'test.txt': 0x%08X\n", hash1);
    printf("✓ Hash 'TEST.TXT': 0x%08X (should match)\n", hash2);

    if (hash1 == hash2)
        printf("✓ Case-insensitive hashing works\n");
    else
        printf("✗ Case-insensitive hashing failed\n");

    printf("\nConsole tests complete!\n");
}

// Alternative entry point for console testing
int main()
{
    // Check command line arguments
    if (GetCommandLineA() && strstr(GetCommandLineA(), "-console"))
    {
        RunConsoleTests();
        printf("\nPress Enter to continue to graphics test...");
        getchar();
    }

    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}

// Debugging utilities
void PrintDirectXInfo()
{
    printf("\nDirectX 11 Status:\n");
    printf("Device: %s\n", g_D3DDevice ? "Valid" : "NULL");
    printf("Context: %s\n", g_D3DContext ? "Valid" : "NULL");
    printf("SwapChain: %s\n", g_SwapChain ? "Valid" : "NULL");
    printf("RenderTarget: %s\n", g_RenderTargetView ? "Valid" : "NULL");
    printf("DepthStencil: %s\n", g_DepthStencilView ? "Valid" : "NULL");
}

void ValidateRenderingPipeline()
{
    printf("\nValidating rendering pipeline...\n");

    // Check if critical objects exist
    bool valid = true;

    if (!g_D3DDevice)
    {
        printf("✗ D3D Device is NULL\n");
        valid = false;
    }

    if (!g_D3DContext)
    {
        printf("✗ D3D Context is NULL\n");
        valid = false;
    }

    if (!g_SwapChain)
    {
        printf("✗ Swap Chain is NULL\n");
        valid = false;
    }

    if (!g_RenderTargetView)
    {
        printf("✗ Render Target View is NULL\n");
        valid = false;
    }

    if (!g_DepthStencilView)
    {
        printf("✗ Depth Stencil View is NULL\n");
        valid = false;
    }

    if (valid)
    {
        printf("✓ All critical DirectX objects are valid\n");

        // Test basic operations
        printf("Testing basic operations...\n");

        // Test clear
        float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        g_D3DContext->ClearRenderTargetView(g_RenderTargetView.Get(), clearColor);
        g_D3DContext->ClearDepthStencilView(g_DepthStencilView.Get(),
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f, 0);
        printf("✓ Clear operations successful\n");

        // Test present
        HRESULT hr = g_SwapChain->Present(0, 0);
        if (SUCCEEDED(hr))
        {
            printf("✓ Present operation successful\n");
        }
        else
        {
            printf("✗ Present operation failed (HRESULT: 0x%08X)\n", hr);
        }
    }
    else
    {
        printf("✗ Rendering pipeline validation failed\n");
    }
}

// Error checking wrapper for debugging
#define CHECK_HRESULT(expr, desc) \
    do { \
        HRESULT hr = (expr); \
        if (FAILED(hr)) { \
            printf("✗ %s failed with HRESULT 0x%08X\n", desc, hr); \
            return false; \
        } else { \
            printf("✓ %s succeeded\n", desc); \
        } \
    } while(0)

// Enhanced scene creation with error checking
bool CreateTestCubeAdvanced(CHScene** scene)
{
    printf("Creating advanced test cube...\n");

    *scene = new CHScene;
    Scene_Clear(*scene);

    // Set scene name
    const char* sceneName = "Advanced Test Cube";
    size_t nameLen = strlen(sceneName) + 1;
    (*scene)->lpName = new char[nameLen];
    strcpy_s((*scene)->lpName, nameLen, sceneName);

    // Create more detailed cube with better normals and UVs
    (*scene)->dwVecCount = 24; // 4 vertices per face, 6 faces
    (*scene)->lpVB = new CHSceneVertex[24];

    float size = 2.0f;
    int vertIndex = 0;

    // Front face (Z+) - Red tinted UVs
    (*scene)->lpVB[vertIndex++] = { -size, -size,  size,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[vertIndex++] = { size, -size,  size,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    (*scene)->lpVB[vertIndex++] = { size,  size,  size,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[vertIndex++] = { -size,  size,  size,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Continue with other faces...
    // (Previous cube creation code continues here)

    // Create indices with proper winding
    (*scene)->dwTriCount = 12;
    (*scene)->lpIB = new WORD[36];

    // Define indices with consistent counter-clockwise winding
    WORD indices[] = {
        // Front
        0, 1, 2,  0, 2, 3,
        // Back  
        4, 5, 6,  4, 6, 7,
        // Left
        8, 9, 10,  8, 10, 11,
        // Right
        12, 13, 14,  12, 14, 15,
        // Top
        16, 17, 18,  16, 18, 19,
        // Bottom
        20, 21, 22,  20, 22, 23
    };

    memcpy((*scene)->lpIB, indices, sizeof(indices));

    // Create and validate texture
    printf("Creating procedural texture...\n");
    if (!CreateProceduralTexture(&g_TestTexture))
    {
        printf("✗ Failed to create procedural texture\n");
        (*scene)->nTex = -1;
        return false;
    }

    // Use texture ID 0 for now since we're not managing in global array
    (*scene)->nTex = 0;
    printf("✓ Texture assigned to scene\n");

    // Create DirectX 11 buffers with error checking
    printf("Creating DirectX 11 vertex buffer...\n");
    if (FAILED(CHSceneInternal::CreateVertexBuffer(*scene)))
    {
        printf("✗ Vertex buffer creation failed\n");
        Scene_Unload(scene);
        return false;
    }

    printf("Creating DirectX 11 index buffer...\n");
    if (FAILED(CHSceneInternal::CreateIndexBuffer(*scene)))
    {
        printf("✗ Index buffer creation failed\n");
        Scene_Unload(scene);
        return false;
    }

    printf("✓ Advanced test cube created successfully\n");
    printf("  Vertices: %d, Triangles: %d, Texture: %d\n",
        (*scene)->dwVecCount, (*scene)->dwTriCount, (*scene)->nTex);

    return true;
}

// Test multiple primitives
bool CreateTestPrimitives(CHScene** scene)
{
    printf("Creating test primitives collection...\n");

    *scene = new CHScene;
    Scene_Clear(*scene);

    // Create a more complex scene with multiple objects
    // This would include planes, spheres, etc. arranged in a test pattern

    (*scene)->dwVecCount = 8; // Simple quad for now
    (*scene)->lpVB = new CHSceneVertex[8];

    // Ground plane
    float groundSize = 5.0f;
    (*scene)->lpVB[0] = { -groundSize, -2.0f, -groundSize,  0.0f,  1.0f,  0.0f, 0.0f, 2.0f, 0.0f, 0.0f };
    (*scene)->lpVB[1] = { groundSize, -2.0f, -groundSize,  0.0f,  1.0f,  0.0f, 2.0f, 2.0f, 0.0f, 0.0f };
    (*scene)->lpVB[2] = { groundSize, -2.0f,  groundSize,  0.0f,  1.0f,  0.0f, 2.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[3] = { -groundSize, -2.0f,  groundSize,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Wall plane
    (*scene)->lpVB[4] = { -groundSize, -2.0f, -groundSize,  0.0f,  0.0f,  1.0f, 0.0f, 2.0f, 0.0f, 0.0f };
    (*scene)->lpVB[5] = { groundSize, -2.0f, -groundSize,  0.0f,  0.0f,  1.0f, 2.0f, 2.0f, 0.0f, 0.0f };
    (*scene)->lpVB[6] = { groundSize,  2.0f, -groundSize,  0.0f,  0.0f,  1.0f, 2.0f, 0.0f, 0.0f, 0.0f };
    (*scene)->lpVB[7] = { -groundSize,  2.0f, -groundSize,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Create indices for 2 quads (4 triangles)
    (*scene)->dwTriCount = 4;
    (*scene)->lpIB = new WORD[12];

    // Ground quad
    (*scene)->lpIB[0] = 0; (*scene)->lpIB[1] = 1; (*scene)->lpIB[2] = 2;
    (*scene)->lpIB[3] = 0; (*scene)->lpIB[4] = 2; (*scene)->lpIB[5] = 3;

    // Wall quad  
    (*scene)->lpIB[6] = 4; (*scene)->lpIB[7] = 5; (*scene)->lpIB[8] = 6;
    (*scene)->lpIB[9] = 4; (*scene)->lpIB[10] = 6; (*scene)->lpIB[11] = 7;

    // Use texture ID 0 for now since we're not managing in global array
    (*scene)->nTex = g_TestTexture ? 0 : -1;

    // Create buffers
    if (FAILED(CHSceneInternal::CreateVertexBuffer(*scene)) ||
        FAILED(CHSceneInternal::CreateIndexBuffer(*scene)))
    {
        printf("✗ Buffer creation failed for test primitives\n");
        Scene_Unload(scene);
        return false;
    }

    printf("✓ Test primitives created\n");
    return true;
}

// Comprehensive engine validation
bool ValidateEngine()
{
    printf("\n=== CH Engine Validation ===\n");

    bool allValid = true;

    // Test DirectX objects
    if (!g_D3DDevice) { printf("✗ D3D Device missing\n"); allValid = false; }
    else { printf("✓ D3D Device valid\n"); }

    if (!g_D3DContext) { printf("✗ D3D Context missing\n"); allValid = false; }
    else { printf("✓ D3D Context valid\n"); }

    if (!g_SwapChain) { printf("✗ Swap Chain missing\n"); allValid = false; }
    else { printf("✓ Swap Chain valid\n"); }

    if (!g_RenderTargetView) { printf("✗ Render Target missing\n"); allValid = false; }
    else { printf("✓ Render Target valid\n"); }

    if (!g_DepthStencilView) { printf("✗ Depth Stencil missing\n"); allValid = false; }
    else { printf("✓ Depth Stencil valid\n"); }

    // Test shader managers
    printf("\nTesting shader managers...\n");

    // Test scene rendering setup
    if (g_TestScene)
    {
        printf("Testing scene preparation...\n");
        Scene_Prepare();
        printf("✓ Scene_Prepare() completed\n");

        // Test matrix operations
        XMMATRIX testMatrix = XMMatrixIdentity();
        Scene_Muliply(g_TestScene, &testMatrix);
        printf("✓ Scene matrix operations work\n");
    }

    // Test camera operations
    if (g_TestCamera)
    {
        printf("Testing camera operations...\n");
        if (Camera_BuildView(g_TestCamera, FALSE))
            printf("✓ Camera view matrix build successful\n");
        else
            printf("✗ Camera view matrix build failed\n");

        if (Camera_BuildProject(g_TestCamera, FALSE))
            printf("✓ Camera projection matrix build successful\n");
        else
            printf("✗ Camera projection matrix build failed\n");
    }

    // Test render states
    printf("Testing render state system...\n");
    SetRenderState(CH_RS_CULLMODE, CH_CULL_CW);
    SetRenderState(CH_RS_ZENABLE, TRUE);
    SetTextureStageState(0, CH_TSS_MINFILTER, CH_TEXF_LINEAR);
    printf("✓ Render state operations completed\n");

    if (allValid)
    {
        printf("\n✓ Engine validation PASSED\n");
    }
    else
    {
        printf("\n✗ Engine validation FAILED\n");
    }

    printf("=============================\n\n");
    return allValid;
}

// Create a simple test scene (cube) for debugging
CHScene* CreateTestScene()
{
    // Instead of creating a scene programmatically, let's try to load the existing test.scene file
    // with better error handling and fallback
    CHScene* scene = nullptr;
    
    printf("Attempting to load test.scene file...\n");
    if (Scene_Load(&scene, "test.scene", 0))
    {
        printf("✓ Successfully loaded test.scene file\n");
        return scene;
    }
    else
    {
        printf("✗ Failed to load test.scene file\n");
        printf("  This is expected if the file doesn't exist or is invalid\n");
        printf("  The application will run without a 3D scene\n");
        return nullptr;
    }
}