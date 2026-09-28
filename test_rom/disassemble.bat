@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "DISASSEMBLER=%REPO_ROOT%\build\Release\msxdisa.exe"

if not exist "%DISASSEMBLER%" set "DISASSEMBLER=%REPO_ROOT%\build\Debug\msxdisa.exe"
if not exist "%DISASSEMBLER%" set "DISASSEMBLER=%REPO_ROOT%\build\msxdisa.exe"

if not exist "%DISASSEMBLER%" (
    echo msxdisa.exe was not found. Build the project with CMake first.
    exit /b 1
)

"%DISASSEMBLER%" --mapper none -o "%SCRIPT_DIR%rabbit_adventure_demo.asm" "%SCRIPT_DIR%rabbit_adventure_demo.rom"
if errorlevel 1 exit /b %errorlevel%

echo Disassembly written to "%SCRIPT_DIR%rabbit_adventure_demo.asm"
