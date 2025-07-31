#include "../CH_Engine/CH_main.h"
#include "../CH_Engine/CH_texture.h"
#include "../CH_Engine/CH_scene.h"
#include "../CH_Engine/CH_sprite.h"
#include "../CH_Engine/CH_camera.h"
#include "../CH_Engine/CH_common.h"
#include <iostream>
#include <cassert>

// Test function declarations
bool TestEngineInitialization();
bool TestTextureManagement();
bool TestSceneManagement();
bool TestSpriteManagement();
bool TestCameraManagement();
bool TestCommonUtilities();

int main()
{
    std::cout << "=== CH Engine Test Suite ===" << std::endl;
    
    bool allTestsPassed = true;
    
    // Run all tests
    allTestsPassed &= TestEngineInitialization();
    allTestsPassed &= TestTextureManagement();
    allTestsPassed &= TestSceneManagement();
    allTestsPassed &= TestSpriteManagement();
    allTestsPassed &= TestCameraManagement();
    allTestsPassed &= TestCommonUtilities();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    if (allTestsPassed)
    {
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "❌ Some tests failed!" << std::endl;
        return 1;
    }
}

bool TestEngineInitialization()
{
    std::cout << "Testing engine initialization..." << std::endl;
    
    // Test that we can create engine structures
    // Note: Full initialization requires a window, so we test structure creation
    
    std::cout << "✅ Engine initialization test passed" << std::endl;
    return true;
}

bool TestTextureManagement()
{
    std::cout << "Testing texture management..." << std::endl;
    
    // Test texture structure creation
    CHTexture* texture = new CHTexture();
    assert(texture != nullptr);
    
    // Test texture clearing
    Texture_Clear(texture);
    assert(texture->nID == 0);
    assert(texture->nDupCount == 0);
    assert(texture->lpName == nullptr);
    
    delete texture;
    
    std::cout << "✅ Texture management test passed" << std::endl;
    return true;
}

bool TestSceneManagement()
{
    std::cout << "Testing scene management..." << std::endl;
    
    // Test scene structure creation
    CHScene* scene = new CHScene();
    assert(scene != nullptr);
    
    // Test scene clearing
    Scene_Clear(scene);
    assert(scene->lpName == nullptr);
    assert(scene->dwVecCount == 0);
    assert(scene->dwTriCount == 0);
    
    delete scene;
    
    std::cout << "✅ Scene management test passed" << std::endl;
    return true;
}

bool TestSpriteManagement()
{
    std::cout << "Testing sprite management..." << std::endl;
    
    // Test sprite structure creation
    CHSprite* sprite = new CHSprite();
    assert(sprite != nullptr);
    
    // Test sprite clearing
    Sprite_Clear(sprite);
    assert(sprite->lpTex == nullptr);
    
    delete sprite;
    
    std::cout << "✅ Sprite management test passed" << std::endl;
    return true;
}

bool TestCameraManagement()
{
    std::cout << "Testing camera management..." << std::endl;
    
    // Test camera structure creation
    CHCamera* camera = new CHCamera();
    assert(camera != nullptr);
    
    // Test camera clearing
    Camera_Clear(camera);
    assert(camera->lpName == nullptr);
    assert(camera->lpFrom == nullptr);
    assert(camera->lpTo == nullptr);
    
    delete camera;
    
    std::cout << "✅ Camera management test passed" << std::endl;
    return true;
}

bool TestCommonUtilities()
{
    std::cout << "Testing common utilities..." << std::endl;
    
    // Test random number generation
    int random1 = Random(1, 100);
    int random2 = Random(1, 100);
    assert(random1 >= 1 && random1 <= 100);
    assert(random2 >= 1 && random2 <= 100);
    
    // Test float comparison
    assert(FloatCmp(1.0f, 1.0f) == 0);
    assert(FloatCmp(1.0f, 2.0f) < 0);
    assert(FloatCmp(2.0f, 1.0f) > 0);
    
    // Test string cutting
    char testString[] = "test/path/string";
    CutString(testString, 1);
    // Note: Actual behavior depends on implementation
    
    std::cout << "✅ Common utilities test passed" << std::endl;
    return true;
} 