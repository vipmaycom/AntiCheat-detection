@echo off
REM build.bat - Build script for reconstructed EasyAntiCheat_EOS.sys v3.2.0 multi-module WDM/Minifilter driver
REM Auto-detects Visual Studio 2022 vcvars64.bat if tools are not in PATH

where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
)

echo [*] Compilando EasyAntiCheat_EOS.sys via MSBuild (WDK Native Tools)...
msbuild eac.sln /nologo /p:Configuration=Release /p:Platform=x64 /v:minimal
if %errorlevel% equ 0 (
    echo [+] Build concluido com sucesso: build\Release\EasyAntiCheat_EOS.sys
) else (
    echo [-] Falha no build.
)
exit /b %errorlevel%
