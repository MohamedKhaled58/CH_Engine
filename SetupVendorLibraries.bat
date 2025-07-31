@echo off
echo ========================================
echo CH Engine - Vendor Libraries Setup
echo ========================================

REM Create vendor directories
if not exist "vendor\DirectXMath" mkdir "vendor\DirectXMath"
if not exist "vendor\spdlog" mkdir "vendor\spdlog"
if not exist "vendor\glm" mkdir "vendor\glm"
if not exist "vendor\stb" mkdir "vendor\stb"

echo Setting up vendor libraries...

REM DirectXMath (usually comes with Windows SDK)
echo Setting up DirectXMath...
if not exist "vendor\DirectXMath\include" (
    echo DirectXMath headers should be available with Windows SDK
    echo If not found, please install Windows 10/11 SDK
) else (
    echo DirectXMath found ✓
)

REM spdlog
echo Setting up spdlog...
if not exist "vendor\spdlog\include\spdlog" (
    echo Downloading spdlog...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.zip' -OutFile 'spdlog.zip'}"
    if exist "spdlog.zip" (
        powershell -Command "& {Expand-Archive -Path 'spdlog.zip' -DestinationPath 'vendor\spdlog_temp' -Force}"
        xcopy "vendor\spdlog_temp\spdlog-1.12.0\*" "vendor\spdlog\" /E /I /Y
        rmdir /S /Q "vendor\spdlog_temp"
        del "spdlog.zip"
        echo spdlog installed ✓
    ) else (
        echo Failed to download spdlog
    )
) else (
    echo spdlog found ✓
)

REM GLM
echo Setting up GLM...
if not exist "vendor\glm\glm" (
    echo Downloading GLM...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip' -OutFile 'glm.zip'}"
    if exist "glm.zip" (
        powershell -Command "& {Expand-Archive -Path 'glm.zip' -DestinationPath 'vendor\glm_temp' -Force}"
        xcopy "vendor\glm_temp\glm-0.9.9.8\glm" "vendor\glm\glm\" /E /I /Y
        rmdir /S /Q "vendor\glm_temp"
        del "glm.zip"
        echo GLM installed ✓
    ) else (
        echo Failed to download GLM
    )
) else (
    echo GLM found ✓
)

REM STB
echo Setting up STB...
if not exist "vendor\stb\stb_image.h" (
    echo Downloading STB...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/nothings/stb/master/stb_image.h' -OutFile 'vendor\stb\stb_image.h'}"
    if exist "vendor\stb\stb_image.h" (
        echo STB installed ✓
    ) else (
        echo Failed to download STB
    )
) else (
    echo STB found ✓
)

echo.
echo ========================================
echo Vendor libraries setup complete!
echo ========================================
echo.
echo Next steps:
echo 1. Run GenerateProjectFiles.bat
echo 2. Open CHEngine.sln in Visual Studio 2022
echo 3. Build the solution
echo.

pause 