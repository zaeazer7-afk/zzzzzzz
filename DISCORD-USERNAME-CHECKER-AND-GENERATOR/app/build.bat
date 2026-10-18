@echo off
echo Installing dependencies...
"C:\Users\zaeaz\AppData\Local\Programs\Python\Python312\python.exe" -m pip install pywebview pyperclip pyinstaller aiohttp aiohttp-socks

set ICON_FLAG=
if exist "icon.ico" set ICON_FLAG=--icon "icon.ico"

echo.
echo Building exe...
"C:\Users\zaeaz\AppData\Local\Programs\Python\Python312\python.exe" -m PyInstaller --noconfirm --onefile --windowed --name "NameVault" --add-data "web;web" --add-data "generators.py;." --add-data "checker_core.py;." --add-data "vault_store.py;." %ICON_FLAG% main.py

if exist "icon.ico" copy /Y "icon.ico" "dist\icon.ico" >nul

echo.
echo Done! exe is in dist\NameVault.exe
echo NOTE: proxies.txt, valid.txt, checked.txt and vault.json will be created next to the exe when you run it.
echo NOTE: keep icon.ico next to the exe so the window/taskbar icon shows up at runtime.
pause
