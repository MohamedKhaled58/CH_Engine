@echo off
echo ========================================
echo CH Engine - Complete Build Script
echo ========================================

REM Check if we're in the right directory
if not exist "CH_Engine" (
    echo ERROR: CH_Engine directory not found!
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

echo Step 1: Setting up vendor libraries...
call SetupVendorLibraries.bat
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to setup vendor libraries!
    pause
    exit /b 1
)

echo.
echo Step 2: Generating project files...
call GenerateProjectFiles.bat
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate project files!
    pause
    exit /b 1
)

echo.
echo Step 3: Building the engine...
if exist "CHEngine.sln" (
    echo Building Debug configuration...
    msbuild CHEngine.sln /p:Configuration=Debug /p:Platform=x64 /m
    
    if %ERRORLEVEL% EQU 0 (
        echo Building Release configuration...
        msbuild CHEngine.sln /p:Configuration=Release /p:Platform=x64 /m
        
        if %ERRORLEVEL% EQU 0 (
            echo Building Debug Static configuration...
            msbuild CHEngine.sln /p:Configuration=Debug_Static /p:Platform=x64 /m
            
            if %ERRORLEVEL% EQU 0 (
                echo Building Release Static configuration...
                msbuild CHEngine.sln /p:Configuration=Release_Static /p:Platform=x64 /m
                
                if %ERRORLEVEL% EQU 0 (
                    echo.
                    echo ========================================
                    echo Build completed successfully!
                    echo ========================================
                    echo.
                    echo Build outputs:
                    echo - bin/Debug/CHEngine.dll
                    echo - bin/Release/CHEngine.dll
                    echo - bin/Debug/CHEngine.lib
                    echo - bin/Release/CHEngine.lib
                    echo.
                    echo Next steps:
                    echo 1. Open CHEngine.sln in Visual Studio 2022
                    echo 2. Run the test project to validate functionality
                    echo 3. Use the engine in your own projects
                    echo.
                ) else (
                    echo ERROR: Failed to build Release Static configuration!
                )
            ) else (
                echo ERROR: Failed to build Debug Static configuration!
            )
        ) else (
            echo ERROR: Failed to build Release configuration!
        )
    ) else (
        echo ERROR: Failed to build Debug configuration!
    )
) else (
    echo ERROR: CHEngine.sln not found! Run GenerateProjectFiles.bat first.
)

echo.
pause 