workspace "CHEngine"
	architecture "x64"
	startproject "TestCHEngine"

	configurations { "Debug", "Release", "Dist" }

	flags { "MultiProcessorCompile" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["DirectXMath"] = "vendor/DirectXMath/Inc"
IncludeDir["spdlog"] = "vendor/spdlog/include"
IncludeDir["glm"] = "vendor/glm"
IncludeDir["stb"] = "vendor/stb"

group "Dependencies"
	-- Add any vendor dependencies here if needed
group ""

project "CH_Engine"
	location "CH_Engine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"WIN32_LEAN_AND_MEAN",
		"NOMINMAX",
		"_WIN32_WINNT=0x0601",
		"CH_CORE_DLL_EXPORTS"
	}

	includedirs {
		"%{prj.name}",
		"%{prj.name}/src",
		"%{prj.name}/include",
		"%{IncludeDir.DirectXMath}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb}"
	}

	links { 
		"d3d11.lib",
		"dxgi.lib",
		"d3dcompiler.lib",
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
		"advapi32.lib",
		"ws2_32.lib",
		"oleaut32.lib"
	}

	postbuildcommands {
		"{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/TestCHEngine"
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
		defines { "CH_PLATFORM_WINDOWS" }

	filter "configurations:Debug"
		defines "CH_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "CH_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CH_DIST"
		runtime "Release"
		optimize "on"

project "TestCHEngine"
	location "TestCHEngine"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"WIN32_LEAN_AND_MEAN",
		"NOMINMAX",
		"_WIN32_WINNT=0x0601"
	}

	includedirs {
		"CH_Engine",
		"CH_Engine/src",
		"CH_Engine/include",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}"
	}

	links {
		"CH_Engine"
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
		defines { "CH_PLATFORM_WINDOWS" }

	filter "configurations:Debug"
		defines "CH_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "CH_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "CH_DIST"
		runtime "Release"
		optimize "on"
