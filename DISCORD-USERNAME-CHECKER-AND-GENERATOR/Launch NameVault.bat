@echo off
cd /d "%~dp0app"
set "PYW=%LOCALAPPDATA%\Programs\Python\Python312\pythonw.exe"
if not exist "%PYW%" (
    echo Could not find pythonw.exe at "%PYW%".
    echo Edit this file and point PYW at your Python install.
    pause
    exit /b 1
)
start "" "%PYW%" main.py
exit
