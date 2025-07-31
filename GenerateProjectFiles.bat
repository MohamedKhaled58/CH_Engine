@echo off
echo ========================================
echo CH Engine - Project File Generator
echo ========================================

REM Check if premake5 exists
if not exist "vendor\bin\Premake\premake5.exe" (
    echo ERROR: premake5.exe not found in vendor\bin\Premake\
    echo Please ensure premake5 is installed in the correct location
    pause
    exit /b 1
)

echo Generating Visual Studio 2022 project files...
vendor\bin\Premake\premake5.exe vs2022

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Project files generated successfully!
    echo ========================================
    echo.
    echo Next steps:
    echo 1. Open CHEngine.sln in Visual Studio 2022
    echo 2. Build the solution (F7 or Ctrl+Shift+B)
    echo 3. Run the example or test projects
    echo.
    echo Build configurations available:
    echo - Debug (Shared Library)
    echo - Release (Shared Library) 
    echo - Debug_Static (Static Library)
    echo - Release_Static (Static Library)
    echo.
) else (
    echo.
    echo ERROR: Failed to generate project files!
    echo Please check the error messages above.
    echo.
)

pause 