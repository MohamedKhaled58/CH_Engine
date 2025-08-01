@echo off
echo ========================================
echo    CHEngine Project Generator
echo ========================================
echo.

REM Use the premake5.exe from vendor/bin/Premake
if exist "vendor\bin\Premake\premake5.exe" (
    echo Using premake5.exe from vendor\bin\Premake\
    vendor\bin\Premake\premake5.exe vs2022
    goto :success
)

REM Fallback to current directory
if exist "premake5.exe" (
    echo Using premake5.exe from current directory
    premake5.exe vs2022
    goto :success
)

echo ERROR: premake5.exe not found!
echo.
echo Please ensure premake5.exe is available in one of these locations:
echo - vendor\bin\Premake\premake5.exe
echo - premake5.exe (current directory)
echo.
pause
exit /b 1

:success
echo.
echo ========================================
echo    Project files generated successfully!
echo ========================================
echo.
echo You can now open CHEngine.sln in Visual Studio 2022
echo.
pause 