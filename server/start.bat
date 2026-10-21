@echo off
setlocal

:: ============================================
::  Trident Auth Server + Discord Bot
::  Set your tokens below, then run this.
:: ============================================

:: --- REQUIRED: Your Discord bot token ---
set DISCORD_TOKEN=YOUR_BOT_TOKEN_HERE

:: --- OPTIONAL: Discord role ID that can manage keys (0 = owner only) ---
set ADMIN_ROLE_ID=0

:: --- API port (loader connects to this) ---
set TRIDENT_PORT=8888

:: ============================================

if "%DISCORD_TOKEN%"=="YOUR_BOT_TOKEN_HERE" (
    echo [!] Edit start.bat and set your DISCORD_TOKEN first.
    pause
    exit /b 1
)

echo [*] Installing dependencies...
pip install -r requirements.txt -q

echo [*] Starting Trident Auth Server + Discord Bot...
echo [*] API: http://0.0.0.0:%TRIDENT_PORT%/api/auth
echo [*] Press Ctrl+C to stop.
echo.

python bot.py

pause
