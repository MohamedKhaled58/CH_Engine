-- CH Engine - DirectX 11 Modern C++ Engine
-- Premake5 Configuration

workspace "CHEngine"
    configurations { "Debug", "Release", "Debug_Static", "Release_Static" }
    platforms { "x64" }
    
    startproject "CHEngine"
    
    -- Global settings
    filter "configurations:Debug*"
        defines { "DEBUG", "_DEBUG" }
        symbols "On"
        runtime "Debug"
        
    filter "configurations:Release*"
        defines { "NDEBUG" }
        optimize "On"
        runtime "Release"
        
    filter "configurations:*_Static"
        kind "StaticLib"
        defines { "CH_CORE_DLL_EXPORTS" }
        
    filter "configurations:Debug"
        kind "SharedLib"
        defines { "CH_CORE_DLL_EXPORTS" }
        
    filter "configurations:Release"
        kind "SharedLib"
        defines { "CH_CORE_DLL_EXPORTS" }
        
    filter "platforms:x64"
        architecture "x64"
        
    -- Output directories
    filter "configurations:Debug*"
        targetdir "bin/Debug"
        objdir "bin-int/Debug"
        
    filter "configurations:Release*"
        targetdir "bin/Release"
        objdir "bin-int/Release"

-- Main Engine Project
project "CHEngine"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    
    -- Source files
    files {
        "CH_Engine/*.h",
        "CH_Engine/*.cpp"
    }
    
    -- Include directories
    includedirs {
        "CH_Engine",
        "vendor/DirectXMath/include",
        "vendor/spdlog/include",
        "vendor/glm",
        "vendor/stb"
    }
    
    -- Preprocessor definitions
    defines {
        "WIN32_LEAN_AND_MEAN",
        "NOMINMAX",
        "UNICODE",
        "_UNICODE",
        "_WIN32_WINNT=0x0601", -- Windows 7+
        "CH_CORE_DLL_EXPORTS"
    }
    
    -- Compiler flags
    filter "configurations:Debug*"
        defines { "DEBUG", "_DEBUG" }
        
    filter "configurations:Release*"
        defines { "NDEBUG" }
        
    -- Platform-specific settings
    filter "system:windows"
        systemversion "latest"
        
    -- DirectX 11 dependencies
    filter "system:windows"
        links {
            "d3d11.lib",
            "dxgi.lib", 
            "d3dcompiler.lib",
            "d3dx11.lib",
            "d3dx9.lib",
            "dinput8.lib",
            "dxguid.lib",
            "xinput.lib",
            "winmm.lib",
            "gdi32.lib",
            "user32.lib",
            "kernel32.lib",
            "shell32.lib",
            "ole32.lib",
            "uuid.lib",
            "comdlg32.lib",
            "advapi32.lib"
        }
        
    -- Warnings and optimizations
    filter "configurations:Debug*"
        flags { "MultiProcessorCompile" }
        
    filter "configurations:Release*"
        flags { "MultiProcessorCompile", "LinkTimeOptimization" }
        
    -- Character set
    characterset "Unicode"
    
    -- Exception handling
    exceptionhandling "On"
    
    -- Runtime library
    filter "configurations:Debug*"
        runtime "Debug"
        
    filter "configurations:Release*"
        runtime "Release"
