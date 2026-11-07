@echo off
setlocal
cd /d "%~dp0"
echo R16F8 build only - never flashes.
where pio >nul 2>nul
if errorlevel 1 (echo ERROR: PlatformIO missing. & pause & exit /b 1)
pio run -d firmware -e m5stack-stopwatch
if errorlevel 1 (echo ERROR: Build failure. & pause & exit /b 2)
echo PASS. App bin generated. Nothing flashed.
pause
