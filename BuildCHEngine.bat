@echo off
REM CH Engine Build and Test Script
REM This script helps build and test the CH Engine

echo ========================================
echo CH Engine Build and Test Script
echo ========================================
echo.

REM Set Visual Studio environment (adjust path as needed)
set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% (
    echo Setting up Visual Studio environment...
    call %VCVARS%
) else (
    echo Warning: Visual Studio environment not found
    echo Please ensure Visual Studio 2019/2022 is installed
    echo.
)

REM Create directories
if not exist "build" mkdir build
if not exist "bin" mkdir bin
if not exist "temp" mkdir temp

echo Creating build directories...
echo   - build\
echo   - bin\
echo   - temp\
echo.

REM Compile CH Engine DLL
echo Compiling CH Engine DLL...
echo ========================

set INCLUDE_DIRS=/I"." /I"CH_Engine\src" /I"TestCHEngine\src"
set DEFINES=/DCH_CORE_DLL_EXPORTS /D_DEBUG /DDEBUG /DUNICODE /D_UNICODE
set LIBS=d3d11.lib dxgi.lib d3dcompiler.lib kernel32.lib user32.lib gdi32.lib winmm.lib

cl.exe /nologo /W3 /EHsc /MDd /Zi %DEFINES% %INCLUDE_DIRS% ^
    CH_Engine\src\CH_main.cpp ^
    CH_Engine\src\CH_common.cpp ^
    CH_Engine\src\CH_texture.cpp ^
    CH_Engine\src\CH_sprite.cpp ^
    CH_Engine\src\CH_scene.cpp ^
    CH_Engine\src\CH_camera.cpp ^
    CH_Engine\src\CH_datafile.cpp ^
    CH_Engine\src\CH_phy.cpp ^
    CH_Engine\src\CH_ptcl.cpp ^
    CH_Engine\src\CH_shape.cpp ^
    CH_Engine\src\CH_font.cpp ^
    CH_Engine\src\CH_key.cpp ^
    CH_Engine\src\CH_omni.cpp ^
    CH_Engine\src\CH_capscreen.cpp ^
    /Fe:"bin\CHEngine.dll" /Fo:"build\\" /Fd:"build\CHEngine.pdb" ^
    /link /DLL /SUBSYSTEM:WINDOWS %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo ✓ CH Engine DLL compiled successfully
) else (
    echo ✗ CH Engine DLL compilation failed
    pause
    exit /b 1
)

echo.

REM Compile Test Application
echo Compiling Test Application...
echo ============================

cl.exe /nologo /W3 /EHsc /MDd /Zi %DEFINES% %INCLUDE_DIRS% ^
    TestCHEngine\src\TestCHEngine.cpp ^
    /Fe:"bin\TestCHEngine.exe" /Fo:"build\\" /Fd:"build\TestCHEngine.pdb" ^
    /link /SUBSYSTEM:CONSOLE "bin\CHEngine.lib" %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo ✓ Test application compiled successfully
) else (
    echo ✗ Test application compilation failed
    echo.
    echo Trying alternative linking...
    
    REM Try linking with the DLL directly
    cl.exe /nologo /W3 /EHsc /MDd /Zi %DEFINES% %INCLUDE_DIRS% ^
        TestCHEngine\src\TestCHEngine.cpp ^
        /Fe:"bin\TestCHEngine.exe" /Fo:"build\\" /Fd:"build\TestCHEngine.pdb" ^
        /link /SUBSYSTEM:CONSOLE %LIBS%
    
    if %ERRORLEVEL% NEQ 0 (
        echo ✗ Alternative linking also failed
        pause
        exit /b 1
    )
)

echo.

REM Copy DLL to output directory
if exist "bin\CHEngine.dll" (
    echo ✓ Copying DLL to output directory...
    copy "bin\CHEngine.dll" "bin\" >nul
)

REM Create test data
echo Creating test data...
echo ====================

REM Create a simple test bitmap (using PowerShell if available)
powershell -Command "& {
    Add-Type -AssemblyName System.Drawing
    $bitmap = New-Object System.Drawing.Bitmap(256, 256)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Blue)
    $brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    for($i = 0; $i -lt 256; $i += 32) {
        for($j = 0; $j -lt 256; $j += 32) {
            if((($i/32) + ($j/32)) % 2 -eq 0) {
                $graphics.FillRectangle($brush, $i, $j, 32, 32)
            }
        }
    }
    $bitmap.Save('bin\test.bmp', [System.Drawing.Imaging.ImageFormat]::Bmp)
    $graphics.Dispose()
    $bitmap.Dispose()
    Write-Host '✓ test.bmp created'
}" 2>nul

if not exist "bin\test.bmp" (
    echo Warning: Could not create test.bmp with PowerShell
    echo You may need to create test textures manually
)

echo.

REM Run tests
echo Running Tests...
echo ===============

echo.
echo Running console tests first...
cd bin
TestCHEngine.exe -console
if %ERRORLEVEL% NEQ 0 (
    echo ✗ Console tests failed
    cd ..
    pause
    exit /b 1
)

echo.
echo Console tests completed. Starting graphics test...
echo Press any key to continue or Ctrl+C to stop...
pause >nul

REM Run graphics test
TestCHEngine.exe
set TEST_RESULT=%ERRORLEVEL%

cd ..

if %TEST_RESULT% EQU 0 (
    echo.
    echo ✓ All tests completed successfully!
) else (
    echo.
    echo ✗ Graphics test returned error code %TEST_RESULT%
)

echo.
echo Build and test complete.
echo.
echo Files created:
echo   - bin\CHEngine.dll     (Main engine)
echo   - bin\CHEngine.lib     (Import library)
echo   - bin\TestCHEngine.exe (Test application)
echo   - bin\test.bmp         (Test texture)
echo   - build\*.obj          (Object files)
echo   - build\*.pdb          (Debug symbols)
echo.

if exist "bin\CHEngine.dll" (
    echo Engine Size: 
    dir /B bin\CHEngine.dll | find /V "Volume"
)

echo.
echo Press any key to exit...
pause >nul