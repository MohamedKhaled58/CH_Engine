#include "../CH_Engine/CH_main.h"
#include "../CH_Engine/CH_texture.h"
#include "../CH_Engine/CH_scene.h"
#include "../CH_Engine/CH_sprite.h"
#include "../CH_Engine/CH_camera.h"
#include <iostream>

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize the engine
    if (Init3D(hInstance, "CH Engine Example", 1024, 768, TRUE, WindowProc, 1) != 0)
    {
        MessageBoxA(NULL, "Failed to initialize CH Engine", "Error", MB_OK);
        return -1;
    }

    // Load a texture
    CHTexture* texture = nullptr;
    if (!Texture_Load(&texture, "example_texture.jpg", 3, CH_POOL_MANAGED, TRUE, 0))
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    // Load a scene
    CHScene* scene = nullptr;
    if (!Scene_Load(&scene, "example_scene.wdf", 0))
    {
        std::cout << "Failed to load scene" << std::endl;
    }

    // Create a camera
    CHCamera* camera = nullptr;
    if (!Camera_Load(&camera, "example_camera.cam", 0))
    {
        std::cout << "Failed to load camera" << std::endl;
    }

    // Main render loop
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process Windows messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Clear the screen
            ClearBuffer(TRUE, TRUE, 0x00000000);

            // Begin 3D rendering
            Begin3D();

            // Set up camera
            if (camera)
            {
                Camera_BuildView(camera, TRUE);
                Camera_BuildProject(camera, TRUE);
            }

            // Draw scene
            if (scene)
            {
                Scene_Draw(scene);
            }

            // End 3D rendering
            End3D();

            // Present the frame
            Flip();
        }
    }

    // Cleanup
    if (texture)
    {
        Texture_Unload(&texture);
    }
    if (scene)
    {
        Scene_Unload(&scene);
    }
    if (camera)
    {
        Camera_Unload(&camera);
    }

    // Shutdown the engine
    Quit3D();

    return 0;
} 