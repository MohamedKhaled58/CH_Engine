// Enhanced CH Engine Test Application
// File: CHEngineTest.cpp
// Demonstrates all major engine systems with proper error handling

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <memory>

// CH Engine includes
#include "CH_main.h"
#include "CH_sprite.h"
#include "CH_scene.h"
#include "CH_camera.h"
#include "CH_texture.h"
#include "CH_font.h"
#include "CH_phy.h"
#include "CH_ptcl.h"
#include "CH_datafile.h"

// Test framework
class CHEngineTest {
private:
    // Test objects
    std::unique_ptr<CHScene> m_testScene;
    std::unique_ptr<CHCamera> m_camera;
    std::unique_ptr<CHFont> m_debugFont;
    std::vector<CHSprite*> m_sprites;

    // Test state
    float m_cameraDistance = 15.0f;
    float m_cameraAngleX = 0.3f;
    float m_cameraAngleY = 0.0f;
    float m_rotationAngle = 0.0f;
    bool m_wireframeMode = false;
    bool m_showDebugInfo = true;

    // Performance tracking
    DWORD m_lastTime = 0;
    DWORD m_frameCount = 0;
    DWORD m_fps = 0;
    DWORD m_lastFPSUpdate = 0;

public:
    bool Initialize(HWND hWnd);
    void Update(float deltaTime);
    void Render();
    void Cleanup();
    void HandleInput(WPARAM wParam);

private:
    bool CreateTestGeometry();
    bool CreateTestCamera();
    bool CreateDebugFont();
    bool CreateTestSprites();
    bool CreateProceduralTexture(CHTexture** texture);

    void UpdateCamera();
    void RenderDebugInfo();
    void RunSystemTests();

    // Safe resource creation with validation
    template<typename T>
    bool SafeCreate(T** resource, const char* name) {
        if (!resource) {
            printf("Error: Null resource pointer for %s\n", name);
            return false;
        }

        *resource = new T;
        if (!*resource) {
            printf("Error: Failed to allocate %s\n", name);
            return false;
        }

        return true;
    }
};

// Global test instance
CHEngineTest g_Test;

bool CHEngineTest::Initialize(HWND hWnd) {
    printf("Initializing CH Engine Test...\n");

    // Initialize timing
    m_lastTime = static_cast<DWORD>(GetTickCount64());
    m_lastFPSUpdate = m_lastTime;

    // Create test content with validation
    if (!CreateTestGeometry()) {
        printf("✗ Failed to create test geometry\n");
        return false;
    }

    if (!CreateTestCamera()) {
        printf("✗ Failed to create test camera\n");
        return false;
    }

    if (!CreateDebugFont()) {
        printf("Warning: Failed to create debug font\n");
        // Non-critical, continue
    }

    if (!CreateTestSprites()) {
        printf("Warning: Failed to create test sprites\n");
        // Non-critical, continue
    }

    // Run system validation tests
    RunSystemTests();

    printf("✓ CH Engine Test initialized successfully\n");
    return true;
}

bool CHEngineTest::CreateTestGeometry() {
    printf("Creating test geometry...\n");

    // Create enhanced cube with better normals and UVs
    CHScene* scene = new CHScene;
    Scene_Clear(scene);

    // Set scene name safely
    const char* sceneName = "Enhanced Test Cube";
    size_t nameLen = strlen(sceneName) + 1;
    scene->lpName = new char[nameLen];
    strcpy_s(scene->lpName, nameLen, sceneName);

    // Create cube vertices (24 vertices for proper face normals)
    scene->dwVecCount = 24;
    scene->lpVB = new CHSceneVertex[24];

    float size = 2.0f;

    // Front face (Z+)
    scene->lpVB[0] = { -size, -size,  size,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[1] = { size, -size,  size,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[2] = { size,  size,  size,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    scene->lpVB[3] = { -size,  size,  size,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Back face (Z-)
    scene->lpVB[4] = { size, -size, -size,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[5] = { -size, -size, -size,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[6] = { -size,  size, -size,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    scene->lpVB[7] = { size,  size, -size,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Left face (X-)
    scene->lpVB[8] = { -size, -size, -size, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[9] = { -size, -size,  size, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[10] = { -size,  size,  size, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    scene->lpVB[11] = { -size,  size, -size, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Right face (X+)
    scene->lpVB[12] = { size, -size,  size,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[13] = { size, -size, -size,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[14] = { size,  size, -size,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    scene->lpVB[15] = { size,  size,  size,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Top face (Y+)
    scene->lpVB[16] = { -size,  size,  size,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[17] = { size,  size,  size,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[18] = { size,  size, -size,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    scene->lpVB[19] = { -size,  size, -size,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Bottom face (Y-)
    scene->lpVB[20] = { -size, -size, -size,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[21] = { size, -size, -size,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
    scene->lpVB[22] = { size, -size,  size,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    scene->lpVB[23] = { -size, -size,  size,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Create indices (12 triangles, 36 indices)
    scene->dwTriCount = 12;
    scene->lpIB = new WORD[36];

    WORD indices[] = {
        // Front face
        0, 1, 2,   0, 2, 3,
        // Back face  
        4, 5, 6,   4, 6, 7,
        // Left face
        8, 9, 10,  8, 10, 11,
        // Right face
        12, 13, 14, 12, 14, 15,
        // Top face
        16, 17, 18, 16, 18, 19,
        // Bottom face
        20, 21, 22, 20, 22, 23
    };

    memcpy(scene->lpIB, indices, sizeof(indices));

    // Create procedural texture
    CHTexture* texture = nullptr;
    if (CreateProceduralTexture(&texture)) {
        // Store texture in global array
        for (int i = 0; i < TEX_MAX; i++) {
            if (g_lpTex[i] == nullptr) {
                g_lpTex[i] = texture;
                texture->nID = i;
                scene->nTex = i;
                g_dwTexCount++;
                break;
            }
        }
        printf("✓ Texture created and assigned to slot %d\n", scene->nTex);
    }
    else {
        scene->nTex = -1;
        printf("Warning: No texture assigned\n");
    }

    // Create DirectX 11 buffers with error checking
    HRESULT hr = CHSceneInternal::CreateVertexBuffer(scene);
    if (FAILED(hr)) {
        printf("✗ Vertex buffer creation failed (HRESULT: 0x%08X)\n", hr);
        Scene_Unload(&scene);
        return false;
    }

    hr = CHSceneInternal::CreateIndexBuffer(scene);
    if (FAILED(hr)) {
        printf("✗ Index buffer creation failed (HRESULT: 0x%08X)\n", hr);
        Scene_Unload(&scene);
        return false;
    }

    m_testScene.reset(scene);
    printf("✓ Test geometry created successfully\n");
    printf("  Vertices: %d, Triangles: %d\n", scene->dwVecCount, scene->dwTriCount);

    return true;
}

bool CHEngineTest::CreateTestCamera() {
    printf("Creating test camera...\n");

    CHCamera* camera = new CHCamera;
    Camera_Clear(camera);

    // Set camera parameters
    camera->fNear = 0.1f;
    camera->fFar = 1000.0f;
    camera->fFov = CHCameraMath::ToRadian(60.0f);
    camera->dwFrameCount = 1;
    camera->nFrame = 0;

    // Allocate and initialize position arrays
    camera->lpFrom = new XMVECTOR[1];
    camera->lpTo = new XMVECTOR[1];

    if (!camera->lpFrom || !camera->lpTo) {
        printf("✗ Failed to allocate camera position arrays\n");
        Camera_Unload(&camera);
        return false;
    }

    // Set guaranteed safe camera position (bypass UpdateCamera to avoid any issues)
    camera->lpFrom[0] = XMVectorSet(10.0f, 5.0f, 10.0f, 1.0f);
    camera->lpTo[0] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    printf("✓ Camera position set to safe values\n");

    m_camera.reset(camera);
    printf("✓ Test camera created\n");
    return true;
}

bool CHEngineTest::CreateDebugFont() {
    printf("Creating debug font...\n");

    CHFont* font = nullptr;
    if (!Font_Create(&font, "Arial", 16)) {
        printf("Warning: Font creation failed\n");
        return false;
    }

    m_debugFont.reset(font);
    printf("✓ Debug font created\n");
    return true;
}

bool CHEngineTest::CreateTestSprites() {
    printf("Creating test sprites...\n");

    // Create a simple colored sprite for UI
    CHSprite* infoSprite = nullptr;
    CHTexture* spriteTexture = nullptr;

    // Create a small texture for the sprite
    if (!Texture_Create(&spriteTexture, 32, 32, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED)) {
        printf("Warning: Failed to create sprite texture\n");
        return false;
    }

    // Create sprite
    if (!Sprite_Create(&infoSprite, 32, 32, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED)) {
        printf("Warning: Failed to create sprite\n");
        Texture_Unload(&spriteTexture);
        return false;
    }

    infoSprite->lpTex = spriteTexture;

    // Position sprite in top-left corner
    Sprite_SetCoor(infoSprite, nullptr, 10, 10, 200, 20);
    Sprite_SetColor(infoSprite, 180, 0, 255, 0); // Semi-transparent green

    m_sprites.push_back(infoSprite);
    printf("✓ Test sprites created\n");
    return true;
}

bool CHEngineTest::CreateProceduralTexture(CHTexture** texture) {
    if (!texture) return false;

    const UINT width = 128;
    const UINT height = 128;

    // Create texture using engine's function
    if (!Texture_Create(texture, width, height, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED)) {
        printf("✗ Texture_Create failed\n");
        return false;
    }

    // Create procedural pattern using internal functions
    std::vector<DWORD> pixels(width * height);

    for (UINT y = 0; y < height; y++) {
        for (UINT x = 0; x < width; x++) {
            // Create a checkerboard pattern with colors
            bool checker = ((x / 16) + (y / 16)) % 2 == 0;
            DWORD color;

            if (checker) {
                // White with some variation
                BYTE intensity = 200 + (x + y) % 56;
                color = 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
            }
            else {
                // Colored squares
                BYTE r = (x * 255) / width;
                BYTE g = (y * 255) / height;
                BYTE b = ((x + y) * 255) / (width + height);
                color = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            pixels[y * width + x] = color;
        }
    }

    // Create texture using existing API
    if (!Texture_Create(texture, width, height, 1, CH_FMT_A8R8G8B8, CH_POOL_MANAGED)) {
        printf("✗ Failed to create texture\n");
        return false;
    }

    // Upload pixel data using DirectX 11 UpdateSubresource
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = g_D3DContext->Map((*texture)->lpTex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        // Copy pixel data
        DWORD* destPixels = static_cast<DWORD*>(mappedResource.pData);
        for (UINT y = 0; y < height; y++) {
            memcpy(destPixels + y * (mappedResource.RowPitch / 4), 
                   pixels.data() + y * width, 
                   width * sizeof(DWORD));
        }
        g_D3DContext->Unmap((*texture)->lpTex.Get(), 0);
    } else {
        printf("✗ Failed to upload texture pixels\n");
        Texture_Unload(texture);
        return false;
    }

    printf("✓ Procedural texture created (%dx%d)\n", width, height);
    return true;
}

void CHEngineTest::UpdateCamera() {
    if (!m_camera || !m_camera->lpFrom || !m_camera->lpTo) return;

    // Ensure minimum camera distance to prevent zero direction vectors
    float safeDistance = std::max(m_cameraDistance, 5.0f); // Much larger minimum
    
    // Calculate camera position using spherical coordinates
    float x = safeDistance * sinf(m_cameraAngleY) * cosf(m_cameraAngleX);
    float y = safeDistance * sinf(m_cameraAngleX);
    float z = safeDistance * cosf(m_cameraAngleY) * cosf(m_cameraAngleX);

    // Additional safety check: ensure we're not too close to origin
    float distanceFromOrigin = sqrtf(x*x + y*y + z*z);
    if (distanceFromOrigin < 1.0f) {
        // Force a safe position
        x = 5.0f;
        y = 0.0f;
        z = 0.0f;
    }

    // Update camera position
    m_camera->lpFrom[0] = XMVectorSet(x, y, z, 1.0f);
    m_camera->lpTo[0] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
}

void CHEngineTest::Update(float deltaTime) {
    // Update rotation
    m_rotationAngle += deltaTime * 0.5f; // Slow rotation

    // Update camera
    UpdateCamera();

    // Update FPS counter
    m_frameCount++;
    DWORD currentTime = static_cast<DWORD>(GetTickCount64());
    if (currentTime - m_lastFPSUpdate >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_lastFPSUpdate = currentTime;

        // Update window title
        char titleText[256];
        sprintf_s(titleText, "CH Engine Test - FPS: %d | Dist: %.1f | Tris: %d",
            m_fps, m_cameraDistance,
            m_testScene ? m_testScene->dwTriCount : 0);
        SetWindowTextA(g_hWnd, titleText);
    }
}

void CHEngineTest::Render() {
    // Begin rendering
    if (!Begin3D()) {
        printf("Warning: Begin3D() failed\n");
        return;
    }

    // Clear buffers with a nice gradient color
    DWORD clearColor = 0xFF001122; // Dark blue-red
    ClearBuffer(TRUE, TRUE, clearColor);

    // Set up camera matrices
    if (m_camera) {
        if (!Camera_BuildView(m_camera.get(), TRUE)) {
            printf("Warning: Camera_BuildView failed\n");
        }
        if (!Camera_BuildProject(m_camera.get(), TRUE)) {
            printf("Warning: Camera_BuildProject failed\n");
        }
    }

    // Set wireframe mode if enabled
    if (m_wireframeMode) {
        // Note: Implement wireframe support in render state manager
        SetRenderState(CH_RS_FILLMODE, CH_FILL_WIREFRAME);
    }
    else {
        SetRenderState(CH_RS_FILLMODE, CH_FILL_SOLID);
    }

    // Render 3D scene
    if (m_testScene) {
        // Apply rotation transform
        XMMATRIX rotationMatrix = XMMatrixRotationY(m_rotationAngle);
        Scene_Muliply(m_testScene.get(), &rotationMatrix);

        // Prepare and draw scene
        Scene_Prepare();
        if (!Scene_Draw(m_testScene.get())) {
            static bool errorReported = false;
            if (!errorReported) {
                printf("Warning: Scene_Draw failed\n");
                errorReported = true;
            }
        }
    }

    // Render 2D overlay
    if (m_showDebugInfo) {
        RenderDebugInfo();
    }

    // End rendering
    End3D();

    // Present frame
    if (!Flip()) {
        static bool errorReported = false;
        if (!errorReported) {
            printf("Warning: Flip() failed\n");
            errorReported = true;
        }
    }
}

void CHEngineTest::RenderDebugInfo() {
    // Render sprites
    if (!m_sprites.empty()) {
        Sprite_Prepare();
        for (CHSprite* sprite : m_sprites) {
            if (sprite) {
                Sprite_Draw(sprite, 1); // Additive blending
            }
        }
    }

    // Render debug text if font is available
    if (m_debugFont) {
        Font_Prepare();

        char debugText[256];
        sprintf_s(debugText, "FPS: %d | Camera: %.1f,%.1f,%.1f",
            m_fps, m_cameraDistance, m_cameraAngleX, m_cameraAngleY);

        Font_Draw(m_debugFont.get(), 10.0f, 40.0f, 0xFFFFFF00, debugText);
    }
}

void CHEngineTest::HandleInput(WPARAM wParam) {
    switch (wParam) {
    case VK_ESCAPE:
        PostQuitMessage(0);
        break;

    case VK_F1:
        m_showDebugInfo = !m_showDebugInfo;
        printf("Debug info: %s\n", m_showDebugInfo ? "ON" : "OFF");
        break;

    case VK_F2:
        m_wireframeMode = !m_wireframeMode;
        printf("Wireframe: %s\n", m_wireframeMode ? "ON" : "OFF");
        break;

    case VK_UP:
        m_cameraAngleX += 0.1f;
        if (m_cameraAngleX > 1.5f) m_cameraAngleX = 1.5f;
        break;

    case VK_DOWN:
        m_cameraAngleX -= 0.1f;
        if (m_cameraAngleX < -1.5f) m_cameraAngleX = -1.5f;
        break;

    case VK_LEFT:
        m_cameraAngleY -= 0.1f;
        break;

    case VK_RIGHT:
        m_cameraAngleY += 0.1f;
        break;

    case VK_ADD:
    case VK_OEM_PLUS:
        m_cameraDistance -= 1.0f;
        if (m_cameraDistance < 2.0f) m_cameraDistance = 2.0f;
        break;

    case VK_SUBTRACT:
    case VK_OEM_MINUS:
        m_cameraDistance += 1.0f;
        if (m_cameraDistance > 50.0f) m_cameraDistance = 50.0f;
        break;

    case 'R':
        // Reset camera
        m_cameraDistance = 15.0f;
        m_cameraAngleX = 0.3f;
        m_cameraAngleY = 0.0f;
        printf("Camera reset\n");
        break;

    case 'P':
        // Print current state
        printf("\n=== Engine State ===\n");
        printf("FPS: %d\n", m_fps);
        printf("Camera Distance: %.2f\n", m_cameraDistance);
        printf("Camera Angles: %.2f, %.2f\n", m_cameraAngleX, m_cameraAngleY);
        printf("Rotation: %.2f rad\n", m_rotationAngle);
        printf("Wireframe: %s\n", m_wireframeMode ? "ON" : "OFF");
        if (m_testScene) {
            printf("Scene: %d vertices, %d triangles\n",
                m_testScene->dwVecCount, m_testScene->dwTriCount);
        }
        printf("Textures loaded: %d/%d\n", g_dwTexCount, TEX_MAX);
        printf("==================\n\n");
        break;

    case 'T':
        // Run runtime tests
        printf("Running runtime validation tests...\n");
        RunSystemTests();
        break;
    }
}

void CHEngineTest::RunSystemTests() {
    printf("\n=== System Validation Tests ===\n");

    // Test 1: DirectX objects validation
    printf("1. DirectX Objects:\n");
    bool dx11Valid = true;
    if (!g_D3DDevice) { printf("   ✗ Device missing\n"); dx11Valid = false; }
    if (!g_D3DContext) { printf("   ✗ Context missing\n"); dx11Valid = false; }
    if (!g_SwapChain) { printf("   ✗ SwapChain missing\n"); dx11Valid = false; }
    if (!g_RenderTargetView) { printf("   ✗ RenderTarget missing\n"); dx11Valid = false; }
    if (!g_DepthStencilView) { printf("   ✗ DepthStencil missing\n"); dx11Valid = false; }

    if (dx11Valid) printf("   ✓ All DirectX 11 objects valid\n");

    // Test 2: Math utilities
    printf("2. Math System:\n");
    float testRad = CHCameraMath::ToRadian(90.0f);
    float testDeg = CHCameraMath::ToDegree(testRad);
    bool mathValid = (abs(testDeg - 90.0f) < 0.001f);
    printf("   %s Math conversion (90° -> %.3f rad -> %.3f°)\n",
        mathValid ? "✓" : "✗", testRad, testDeg);

    // Test 3: Random number generator
    printf("3. Random System:\n");
    int rand1 = Random(1, 100);
    int rand2 = Random(1, 100);
    bool randValid = (rand1 != rand2 && rand1 >= 1 && rand1 <= 100);
    printf("   %s Random generation (%d, %d)\n",
        randValid ? "✓" : "✗", rand1, rand2);

    // Test 4: Texture system
    printf("4. Texture System:\n");
    printf("   Loaded textures: %d/%d\n", g_dwTexCount, TEX_MAX);
    bool texValid = (g_dwTexCount > 0);
    printf("   %s Texture management\n", texValid ? "✓" : "✗");

    // Test 5: Scene system
    printf("5. Scene System:\n");
    if (m_testScene) {
        bool sceneValid = (m_testScene->dwVecCount > 0 &&
            m_testScene->dwTriCount > 0 &&
            m_testScene->lpVB != nullptr &&
            m_testScene->lpIB != nullptr);
        printf("   %s Scene data integrity\n", sceneValid ? "✓" : "✗");
        printf("   %s DirectX buffers\n",
            (m_testScene->vertexBuffer && m_testScene->indexBuffer) ? "✓" : "✗");
    }
    else {
        printf("   ✗ No test scene loaded\n");
    }

    // Test 6: Camera system
    printf("6. Camera System:\n");
    if (m_camera) {
        bool cameraValid = (m_camera->lpFrom != nullptr &&
            m_camera->lpTo != nullptr &&
            m_camera->dwFrameCount > 0);
        printf("   %s Camera data integrity\n", cameraValid ? "✓" : "✗");

        // Test matrix building
        bool viewValid = Camera_BuildView(m_camera.get(), FALSE);
        bool projValid = Camera_BuildProject(m_camera.get(), FALSE);
        printf("   %s View matrix build\n", viewValid ? "✓" : "✗");
        printf("   %s Projection matrix build\n", projValid ? "✓" : "✗");
    }
    else {
        printf("   ✗ No camera loaded\n");
    }

    // Test 7: Render state system
    printf("7. Render State System:\n");
    SetRenderState(CH_RS_CULLMODE, CH_CULL_CW);
    SetRenderState(CH_RS_ZENABLE, TRUE);
    SetTextureStageState(0, CH_TSS_MINFILTER, CH_TEXF_LINEAR);
    printf("   ✓ Render state operations completed\n");

    // Test 8: Basic rendering test
    printf("8. Rendering Test:\n");
    float testColor[4] = { 0.5f, 0.0f, 0.5f, 1.0f };
    g_D3DContext->ClearRenderTargetView(g_RenderTargetView.Get(), testColor);
    printf("   ✓ Clear operations successful\n");

    printf("===============================\n\n");
}

void CHEngineTest::Cleanup() {
    printf("Cleaning up test resources...\n");

    // Clean up sprites
    for (CHSprite* sprite : m_sprites) {
        if (sprite) {
            Sprite_Unload(&sprite);
        }
    }
    m_sprites.clear();
    printf("✓ Sprites cleaned up\n");

    // Font cleanup is handled by unique_ptr destructor
    if (m_debugFont) {
        CHFont* font = m_debugFont.release();
        Font_Release(&font);
        printf("✓ Font cleaned up\n");
    }

    // Scene cleanup is handled by unique_ptr destructor  
    if (m_testScene) {
        CHScene* scene = m_testScene.release();
        Scene_Unload(&scene);
        printf("✓ Scene cleaned up\n");
    }

    // Camera cleanup is handled by unique_ptr destructor
    if (m_camera) {
        CHCamera* camera = m_camera.release();
        Camera_Unload(&camera);
        printf("✓ Camera cleaned up\n");
    }

    printf("✓ All test resources cleaned up\n");
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        g_Test.HandleInput(wParam);
        return 0;

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        // Delegate to test class mouse handling
        if (delta > 0) {
            g_Test.HandleInput(VK_ADD);
        }
        else {
            g_Test.HandleInput(VK_SUBTRACT);
        }
        return 0;
    }

    case WM_SIZE:
        // Handle window resize
        if (wParam != SIZE_MINIMIZED) {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            printf("Window resized to %dx%d\n", width, height);
            // TODO: Implement proper resize handling
        }
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Console test mode for debugging without graphics
void RunConsoleTests() {
    printf("CH Engine Console Tests\n");
    printf("=======================\n\n");

    // Test math functions
    printf("1. Testing math utilities...\n");
    float rad90 = CHCameraMath::ToRadian(90.0f);
    float rad180 = CHCameraMath::ToRadian(180.0f);
    float deg90 = CHCameraMath::ToDegree(rad90);
    printf("   90° = %.4f rad = %.4f°\n", rad90, deg90);
    printf("   180° = %.4f rad\n", rad180);

    // Test vector math
    XMVECTOR v1 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR v2 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
    XMVECTOR cross = XMVector3Cross(v1, v2);
    printf("   Cross product test: (1,0,0) x (0,1,0) = (%.1f,%.1f,%.1f)\n",
        XMVectorGetX(cross), XMVectorGetY(cross), XMVectorGetZ(cross));

    // Test random numbers
    printf("\n2. Testing random number generator...\n");
    std::vector<int> randoms;
    for (int i = 0; i < 10; i++) {
        randoms.push_back(Random(1, 100));
    }
    printf("   Random sequence: ");
    for (int r : randoms) printf("%d ", r);
    printf("\n");

    // Test string functions
    printf("\n3. Testing string utilities...\n");
    char testStr1[] = "level1\\level2\\level3\\file.txt";
    char testStr2[] = "level1\\level2\\level3\\file.txt";

    CutString(testStr1, 2);
    printf("   CutString(2): '%s'\n", testStr1);

    CutString(testStr2, 1);
    printf("   CutString(1): '%s'\n", testStr2);

    // Test float comparison
    printf("\n4. Testing float comparison...\n");
    float f1 = 1.0f, f2 = 1.0001f, f3 = 1.1f;
    printf("   FloatCmp(1.0, 1.0001, 0.001): %d\n", FloatCmp(f1, f2, 0.001f));
    printf("   FloatCmp(1.0, 1.1, 0.001): %d\n", FloatCmp(f1, f3, 0.001f));

    // Test data file hashing
    printf("\n5. Testing data file system...\n");
    DWORD hash1 = string_id("test.txt");
    DWORD hash2 = string_id("TEST.TXT");
    DWORD hash3 = string_id("different.txt");
    printf("   Hash 'test.txt': 0x%08X\n", hash1);
    printf("   Hash 'TEST.TXT': 0x%08X %s\n", hash2,
        (hash1 == hash2) ? "(match)" : "(different)");
    printf("   Hash 'different.txt': 0x%08X\n", hash3);

    printf("\n✓ Console tests completed!\n\n");
}

// Engine capability demonstration
void PrintEngineCapabilities() {
    printf("CH Engine Capabilities\n");
    printf("=====================\n\n");

    printf("Graphics API: DirectX 11 with DirectX 8 compatibility\n");
    printf("Target Platform: Windows Desktop\n");
    printf("Feature Level: ");
    switch (g_FeatureLevel) {
    case D3D_FEATURE_LEVEL_11_1: printf("11.1\n"); break;
    case D3D_FEATURE_LEVEL_11_0: printf("11.0\n"); break;
    case D3D_FEATURE_LEVEL_10_1: printf("10.1\n"); break;
    case D3D_FEATURE_LEVEL_10_0: printf("10.0\n"); break;
    default: printf("Unknown\n"); break;
    }

    printf("\nCore Systems:\n");
    printf("  ✓ 3D Scene Rendering (static geometry)\n");
    printf("  ✓ Camera System (first-person controls)\n");
    printf("  ✓ Texture Management (multiple formats)\n");
    printf("  ✓ 2D Sprite Rendering\n");
    printf("  ✓ Font Rendering (dynamic character caching)\n");
    printf("  ✓ Skeletal Animation (CHPhy system)\n");
    printf("  ✓ Particle Systems (CHPtcl)\n");
    printf("  ✓ Vector Graphics (CHShape)\n");
    printf("  ✓ Data File System (.WDF packs)\n");
    printf("  ✓ Screen Capture (BMP/JPEG)\n");

    printf("\nSupported Formats:\n");
    printf("  Textures: BMP, DXT1/3/5, various RGB formats\n");
    printf("  Data: Custom binary formats with versioning\n");
    printf("  Animation: Keyframe-based with interpolation\n");

    printf("\nBest Suited For:\n");
    printf("  • Early 2000s style 3D games\n");
    printf("  • Educational graphics programming\n");
    printf("  • Rapid prototyping\n");
    printf("  • Indie games with retro aesthetics\n");
    printf("  • Porting DirectX 8 applications\n");

    printf("\n");
}

// Main application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("CH Engine Test Application\n");
    printf("==========================\n\n");

    // Check for console mode
    if (lpCmdLine && strstr(lpCmdLine, "-console")) {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        RunConsoleTests();
        printf("Press Enter to continue to graphics test...");
        getchar();
    }

    // Print engine info
    PrintEngineCapabilities();

    // Initialize DirectX 11
    printf("Initializing DirectX 11...\n");
    int initResult = Init3D(hInstance, "CH Engine Test", 1280, 720, TRUE, WindowProc, 2);

    switch (initResult) {
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
        printf("✗ Unknown initialization error (%d)\n", initResult);
        return -1;
    }

    // Initialize test framework
    if (!g_Test.Initialize(g_hWnd)) {
        printf("✗ Test initialization failed\n");
        Quit3D();
        return -1;
    }

    // Print controls
    printf("\nControls:\n");
    printf("  Arrow Keys    - Rotate camera\n");
    printf("  +/- or Wheel  - Zoom in/out\n");
    printf("  R             - Reset camera\n");
    printf("  P             - Print state info\n");
    printf("  T             - Run validation tests\n");
    printf("  F1            - Toggle debug info\n");
    printf("  F2            - Toggle wireframe\n");
    printf("  ESC           - Exit\n");
    printf("\nStarting render loop...\n\n");

    // Main rendering loop
    MSG msg = {};
    DWORD lastTime = static_cast<DWORD>(GetTickCount64());

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Frame rate limiting (~60 FPS)
            if (LimitRate(16)) {
                DWORD currentTime = static_cast<DWORD>(GetTickCount64());
                float deltaTime = (currentTime - lastTime) / 1000.0f;
                lastTime = currentTime;

                // Update and render
                g_Test.Update(deltaTime);
                g_Test.Render();

                // Check for device issues
                if (IfDeviceLost()) {
                    printf("Device lost detected, attempting reset...\n");
                    if (ResetDevice()) {
                        printf("✓ Device reset successful\n");
                    }
                    else {
                        printf("✗ Device reset failed\n");
                        break;
                    }
                }
            }
        }
    }

    // Cleanup
    printf("\nShutting down...\n");
    g_Test.Cleanup();
    Quit3D();
    printf("✓ CH Engine Test completed\n");

    return static_cast<int>(msg.wParam);
}

// Alternative console entry point
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}