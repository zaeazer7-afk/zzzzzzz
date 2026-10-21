@echo off
setlocal

echo ========================================
echo  Trident Loader Build
echo ========================================
echo.

:: Step 1 — Check trident.dll exists
set DLL_PATH=build\bin\Release\trident.dll
if not exist "%DLL_PATH%" (
    echo [!] trident.dll not found at %DLL_PATH%
    echo [!] Build the Trident project in Release|x64 first.
    echo.
    pause
    exit /b 1
)

echo [1/3] Found %DLL_PATH%

:: Step 2 — Run embed_dll.py to generate payload.hpp
echo [2/3] Encrypting DLL into payload.hpp...

where python >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [!] Python not found. Install Python 3 and add to PATH.
    pause
    exit /b 1
)

python loader\embed_dll.py "%DLL_PATH%" loader\payload.hpp
if %ERRORLEVEL% neq 0 (
    echo [!] embed_dll.py failed.
    pause
    exit /b 1
)

echo [3/3] Building loader...

:: Step 3 — Build loader via MSBuild
set MSBUILD=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do set MSBUILD=%%i

if "%MSBUILD%"=="" (
    echo [!] MSBuild not found. Open a Developer Command Prompt or install VS Build Tools.
    echo [!] Alternatively, open Trident.sln and build the Loader project manually.
    pause
    exit /b 1
)

"%MSBUILD%" loader\Loader.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if %ERRORLEVEL% neq 0 (
    echo.
    echo [!] Loader build failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Done!
echo  Output: build\bin\Release\trident_loader.exe
echo ========================================
echo.
pause
