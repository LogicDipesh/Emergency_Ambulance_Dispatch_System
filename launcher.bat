@echo off
rem One-click launcher for the Emergency Ambulance Dispatch System.
rem Run build.bat first so dispatch.dll exists.
rem Works from Explorer (double-click), cmd, or PowerShell.

setlocal
pushd "%~dp0"

if not exist "dispatch.dll" (
    echo [ERROR] dispatch.dll not found. Run build.bat first.
    pause
    exit /b 1
)

where python >nul 2>nul
if errorlevel 1 (
    echo [ERROR] python not found on PATH.
    pause
    exit /b 1
)

python -c "import flask" >nul 2>nul
if errorlevel 1 (
    echo Installing Flask ...
    python -m pip install flask
)

echo Starting server - open http://127.0.0.1:5000/ in your browser
start "" "http://127.0.0.1:5000/"
python "flask\app.py"

popd
pause
