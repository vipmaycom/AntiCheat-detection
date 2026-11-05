@echo off
setlocal enabledelayedexpansion

REM build.bat - Automated build script for Vanguard Anti-Tamper Module (vgm.exe v1.19.1.18)
echo [*] Localizando ferramentas de compilacao do Visual Studio 2022...

set "VS_PATH="
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VS_PATH%"=="" (
    echo [-] Erro: vcvars64.bat nao encontrado.
    exit /b 1
)

call "%VS_PATH%" >nul 2>&1
echo [*] Compilando vgm.exe via MSBuild...

msbuild.exe vgm.sln /p:Configuration=Release /p:Platform=x64 /nologo /v:m
if %ERRORLEVEL% NEQ 0 (
    echo [-] Falha na compilacao do modulo vgm.exe!
    exit /b %ERRORLEVEL%
)

echo [+] Build concluido com sucesso: build\Release\vgm.exe
exit /b 0
