@echo off
echo Installing dependencies...
pip install requests pyinstaller
echo.
echo Building .exe...
pyinstaller --onefile --windowed --name "LinkBypass" --icon=NONE gui.py
echo.
echo Done! Find LinkBypass.exe in the dist\ folder.
pause
