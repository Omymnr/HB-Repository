@echo off
REM ========================================================
REM   Script para compilar HelbreathLauncher.exe
REM   Requiere Visual Studio (cl.exe, rc.exe en PATH)
REM ========================================================

echo ============================================
echo   Compilando Helbreath Launcher
echo ============================================
echo.

REM Verificar que existe el icono
if not exist helbreath.ico (
    echo ERROR: No se encuentra helbreath.ico
    echo Copia el icono del juego a esta carpeta
    pause
    exit /b 1
)

REM Compilar recursos
echo Compilando recursos...
rc /nologo HelbreathLauncher.rc
if errorlevel 1 (
    echo ERROR: Fallo al compilar recursos
    pause
    exit /b 1
)

REM Compilar codigo
echo Compilando codigo...
cl /nologo /EHsc /O2 /DWIN32 /D_WINDOWS /DNDEBUG ^
   HelbreathLauncher.cpp HelbreathLauncher.res ^
   /link /SUBSYSTEM:WINDOWS /OUT:HelbreathLauncher.exe ^
   user32.lib gdi32.lib comctl32.lib gdiplus.lib wininet.lib crypt32.lib advapi32.lib shell32.lib

if errorlevel 1 (
    echo ERROR: Fallo al compilar
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Compilacion exitosa!
echo   Archivo: HelbreathLauncher.exe
echo ============================================

REM Limpiar archivos temporales
del /q HelbreathLauncher.obj 2>nul
del /q HelbreathLauncher.res 2>nul

pause
