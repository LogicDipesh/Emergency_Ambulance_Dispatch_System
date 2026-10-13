@echo off
rem =====================================================================
rem Build script for the Emergency Ambulance Dispatch System.
rem Produces:  dispatch.dll  (C engine, loaded by Flask via ctypes)
rem            text_main.exe (console demo driver)
rem Requires:  MinGW-w64 gcc on PATH.
rem
rem The DLL must match Python's bitness: a 64-bit Python cannot load a
rem 32-bit DLL ("OSError: [WinError 193] %1 is not a valid Win32
rem application"). This script detects the bitness of both Python and gcc,
rem adapts with -m32/-m64 when the compiler supports it, and verifies at
rem the end that this Python can actually load the DLL.
rem =====================================================================

setlocal EnableDelayedExpansion

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] gcc not found on PATH. Install MinGW-w64.
    exit /b 1
)
where python >nul 2>nul
if errorlevel 1 (
    echo [ERROR] python not found on PATH.
    exit /b 1
)

for /f %%i in ('python -c "import struct;print(struct.calcsize('P')*8)"') do set PYBITS=%%i
for /f %%i in ('gcc -dumpmachine') do set TARGET=%%i
if not defined PYBITS (
    echo [ERROR] Could not detect Python bitness.
    exit /b 1
)
set GCCBITS=32
echo !TARGET! | findstr /i "x86_64" >nul && set GCCBITS=64
echo !TARGET! | findstr /i "aarch64" >nul && set GCCBITS=64
echo Python: !PYBITS!-bit    gcc default: !GCCBITS!-bit ^(!TARGET!^)

set FLAG=
if "!PYBITS!"=="!GCCBITS!" goto haveflag

rem --- mismatch: check whether this gcc can target the other bitness ---
if "!PYBITS!"=="64" ( set FLAG=-m64 ) else ( set FLAG=-m32 )
echo [i] Bitness mismatch ^(Python !PYBITS!-bit vs gcc !GCCBITS!-bit^) - testing !FLAG! ...
>"%TEMP%\archtest.c" echo int main^(void^){return 0;}
gcc !FLAG! "%TEMP%\archtest.c" -o "%TEMP%\archtest.exe" >nul 2>&1
if errorlevel 1 (
    del "%TEMP%\archtest.c" 2>nul
    goto noflag
)
del "%TEMP%\archtest.c" 2>nul
del "%TEMP%\archtest.exe" 2>nul
echo [+] gcc accepts !FLAG! - building !PYBITS!-bit binaries.
goto haveflag

:noflag
echo [ERROR] Your Python is !PYBITS!-bit, but this gcc can only build !GCCBITS!-bit binaries.
echo         A 64-bit Python cannot load a 32-bit DLL ^(and vice versa^).
echo         Fix, pick one:
echo           - Install a 64-bit MinGW-w64 ^(e.g. the x86_64 UCRT build from https://winlibs.com/^),
echo             make sure its bin folder is FIRST on PATH, then re-run build.bat; or
echo           - Install a !GCCBITS!-bit Python instead.
exit /b 1

:haveflag
echo [1/3] Building dispatch.dll ...
gcc !FLAG! -O2 -std=c99 -shared -o dispatch.dll c\heap.c c\graph.c c\city.c c\dijkstra.c c\emerg.c c\ambtable.c c\engine.c
if errorlevel 1 (
    echo [ERROR] dispatch.dll build failed.
    exit /b 1
)

echo [2/3] Building text_main.exe ...
gcc !FLAG! -O2 -std=c99 -o text_main.exe c\text_main.c c\heap.c c\graph.c c\city.c c\dijkstra.c c\emerg.c c\ambtable.c c\engine.c
if errorlevel 1 (
    echo [ERROR] text_main.exe build failed.
    exit /b 1
)

echo [3/3] Verifying this Python can load dispatch.dll ...
python -c "import ctypes, os; ctypes.CDLL(os.path.abspath('dispatch.dll')); print('dispatch.dll loads OK')"
if errorlevel 1 (
    echo [ERROR] dispatch.dll built, but this Python still cannot load it.
    exit /b 1
)

echo.
echo Build OK: dispatch.dll, text_main.exe
