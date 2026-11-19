@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
set "PORT=COM16"
if not "%~1"=="" set "PORT=%~1"
set "BIN=%~dp0firmware\.pio\build\m5stack-stopwatch\firmware.bin"
set "PART_DUMP=%TEMP%\star_voyager_R16F8_ota_partition_!RANDOM!.bin"
set "OTA_DUMP=%TEMP%\star_voyager_R16F8_otadata_!RANDOM!.bin"
set "TARGET_FILE=%TEMP%\voygerm5_app_target_!RANDOM!.txt"
set "HEADER_DUMP=%TEMP%\voygerm5_old_app_header_!RANDOM!.bin"
echo ============================================================
echo voygerM5 - M5 0.3.0 / guarded APP-ONLY on !PORT!
echo Verify live partitions and OTA boot selection. Do not assume a two-slot layout.
echo Factory-only or recognized OTA layout: flash ONLY the validated boot APP.
echo Existing bootloader, partition table, OTA data, NVS and storage untouched.
echo ============================================================
where pio >nul 2>nul
if errorlevel 1 (echo ERROR: pio missing. NOTHING FLASHED. & pause & exit /b 1)
python -m esptool version >nul 2>nul
if errorlevel 1 (echo ERROR: esptool Python package missing. NOTHING FLASHED. & pause & exit /b 1)
pio run -d firmware -e m5stack-stopwatch
if errorlevel 1 (echo ERROR: build failed. NOTHING FLASHED. & pause & exit /b 3)
if not exist "!BIN!" (echo ERROR: no app BIN. NOTHING FLASHED. & pause & exit /b 4)
echo Reading current partition table from device (4096 bytes, no changes)...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after no-reset read-flash 0x8000 0x1000 "!PART_DUMP!"
if errorlevel 1 (echo ERROR: Could not read partition table. NOTHING FLASHED. & pause & exit /b 6)
echo Reading OTA boot selection data (8192 bytes, no changes)...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after no-reset read-flash 0xD000 0x2000 "!OTA_DUMP!"
if errorlevel 1 (echo ERROR: Could not read otadata. NOTHING FLASHED. & pause & exit /b 9)
python tools\verify_ota0.py --dump "!PART_DUMP!" --otadata "!OTA_DUMP!" --bin "!BIN!" --target-file "!TARGET_FILE!"
if errorlevel 1 (echo ERROR: Live partition, boot slot or image verification failed. NOTHING FLASHED. & pause & exit /b 7)
set "APP_OFFSET="
set "APP_SIZE="
for /f "usebackq tokens=1,2" %%A in ("!TARGET_FILE!") do (
    set "APP_OFFSET=%%A"
    set "APP_SIZE=%%B"
)
if not defined APP_OFFSET (echo ERROR: Missing validated APP offset. NOTHING FLASHED. & pause & exit /b 10)
if not defined APP_SIZE (echo ERROR: Missing validated APP size. NOTHING FLASHED. & pause & exit /b 10)
echo Checking existing APP header at !APP_OFFSET! ^(read-only^)...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after no-reset read-flash !APP_OFFSET! 0x20 "!HEADER_DUMP!"
if errorlevel 1 (echo ERROR: Could not read existing APP header. NOTHING FLASHED. & pause & exit /b 11)
python tools\verify_ota0.py --dump "!PART_DUMP!" --otadata "!OTA_DUMP!" --bin "!BIN!" --target-file "!TARGET_FILE!" --existing-header "!HEADER_DUMP!"
if errorlevel 1 (echo ERROR: Existing APP header verification failed. NOTHING FLASHED. & pause & exit /b 12)
del /q "!PART_DUMP!" "!OTA_DUMP!" "!TARGET_FILE!" "!HEADER_DUMP!" 2>nul
echo FLASHING VERIFIED APP ONLY at !APP_OFFSET! size !APP_SIZE! on !PORT!...
python -m esptool --chip esp32s3 -p !PORT! -b 460800 --before default-reset --after hard-reset write-flash !APP_OFFSET! "!BIN!"
if errorlevel 1 (echo ERROR: APP flash failed. Do not erase flash. & pause & exit /b 8)
echo PASS: verified APP written at !APP_OFFSET!. Device reset.
pause
exit /b 0
