@echo off
if exist C:\msys64\mingw64\bin set PATH=C:\msys64\mingw64\bin;%PATH%
if exist C:\msys64\usr\bin set PATH=C:\msys64\usr\bin;%PATH%


where mingw32-make >nul 2>nul
if %ERRORLEVEL% equ 0 (
    mingw32-make %*
    exit /b %ERRORLEVEL%
)

where make >nul 2>nul
if %ERRORLEVEL% equ 0 (
    make %*
    exit /b %ERRORLEVEL%
)

echo Erro: Nenhum comando 'make' ou 'mingw32-make' foi encontrado no seu sistema.
echo Instale o Make (ex: via MSYS2: pacman -S make) ou adicione-o ao seu PATH.
exit /b 1



