@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
set "PORT=COM16"
if not "%~1"=="" set "PORT=%~1"
if /I "!PORT!"=="COM13" (
    echo ERROR: COM13 is the old port. NOTHING FLASHED.
    pause
    exit /b 5
)
set "BIN=%~dp0firmware\.pio\build\m5stack-stopwatch\firmware.bin"
set "PART_DUMP=%TEMP%\star_voyager_R16F8_ota_partition_!RANDOM!.bin"
set "OTA_DUMP=%TEMP%\star_voyager_R16F8_otadata_!RANDOM!.bin"
echo ============================================================
echo STAR VOYAGER R16F8 - M5 0.3.0 / OTA0 on !PORT!
echo Existing ESP-IDF boot: ota_0=0x20000, ota_1=0x510000.
echo Only ota_0 app image will be flashed. No chip erase.
echo Existing bootloader, partition table, OTA data, NVS and storage untouched.
echo ============================================================
where pio >nul 2>nul
if errorlevel 1 (echo ERROR: pio missing. NOTHING FLASHED. & pause & exit /b 1)
python -m esptool version >nul 2>nul
if errorlevel 1 (echo ERROR: esptool Python package missing. NOTHING FLASHED. & pause & exit /b 1)
pio run -d firmware -e m5stack-stopwatch -t clean
if errorlevel 1 (echo ERROR: clean failed. NOTHING FLASHED. & pause & exit /b 2)
pio run -d firmware -e m5stack-stopwatch
if errorlevel 1 (echo ERROR: build failed. NOTHING FLASHED. & pause & exit /b 3)
if not exist "!BIN!" (echo ERROR: no app BIN. NOTHING FLASHED. & pause & exit /b 4)
echo Reading current partition table from device (4096 bytes, no changes)...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after no-reset read-flash 0x8000 0x1000 "!PART_DUMP!"
if errorlevel 1 (echo ERROR: Could not read partition table. NOTHING FLASHED. & pause & exit /b 6)
echo Reading OTA boot selection data (8192 bytes, no changes)...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after no-reset read-flash 0xD000 0x2000 "!OTA_DUMP!"
if errorlevel 1 (echo ERROR: Could not read otadata. NOTHING FLASHED. & pause & exit /b 9)
python tools\verify_ota0.py --dump "!PART_DUMP!" --otadata "!OTA_DUMP!" --bin "!BIN!"
if errorlevel 1 (echo ERROR: Partition mismatch; abort. NOTHING FLASHED. & pause & exit /b 7)
del /q "!PART_DUMP!" "!OTA_DUMP!" 2>nul
echo FLASHING APP ONLY into ota_0 at 0x20000 on !PORT!...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after hard-reset write-flash 0x20000 "!BIN!"
if errorlevel 1 (echo ERROR: APP flash failed. Do not erase flash. & pause & exit /b 8)
echo PASS: app written to ota_0 at 0x20000. Device reset.
pause
exit /b 0
