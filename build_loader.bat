@echo off
setlocal

echo ========================================
echo  Trident Loader Build
echo ========================================
echo.

:: Find MSBuild
set MSBUILD=
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do set MSBUILD=%%i

if "%MSBUILD%"=="" (
    echo [!] MSBuild not found. Open a Developer Command Prompt or install VS Build Tools.
    pause
    exit /b 1
)

:: Step 1 — Check trident.dll exists
set DLL_PATH=build\bin\Release\trident.dll
if not exist "%DLL_PATH%" (
    echo [!] trident.dll not found at %DLL_PATH%
    echo [!] Build the Trident project in Release|x64 first.
    pause
    exit /b 1
)
echo [1/4] Found %DLL_PATH%

:: Step 2 — Build embed_dll.exe
echo [2/4] Building embed tool...
"%MSBUILD%" loader\EmbedDll.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if %ERRORLEVEL% neq 0 (
    echo [!] EmbedDll build failed.
    pause
    exit /b 1
)

:: Step 3 — Run embed_dll.exe to encrypt DLL into payload.hpp
echo [3/4] Encrypting DLL into payload.hpp...
build\bin\Release\embed_dll.exe "%DLL_PATH%" loader\payload.hpp
if %ERRORLEVEL% neq 0 (
    echo [!] embed_dll.exe failed.
    pause
    exit /b 1
)

:: Step 4 — Build loader
echo [4/4] Building loader...
"%MSBUILD%" loader\Loader.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if %ERRORLEVEL% neq 0 (
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
