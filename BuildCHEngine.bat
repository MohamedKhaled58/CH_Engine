@echo off
echo Building CH_Engine project only...
cd /d "%~dp0"

if exist "vendor\bin\Premake\premake5.exe" (
    echo Regenerating project files...
    vendor\bin\Premake\premake5.exe vs2022
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to generate project files
        pause
        exit /b 1
    )
)

if exist "CH_Engine.sln" (
    echo Building CH_Engine project...
    msbuild CH_Engine.sln /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /verbosity:minimal
    if %ERRORLEVEL% NEQ 0 (
        echo Build failed
        pause
        exit /b 1
    )
    echo Build successful!
) else (
    echo Solution file not found
    pause
    exit /b 1
)

pause 