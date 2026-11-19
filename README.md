# voygerM5 — Space flight simulator for M5Stack StopWatch

Spaceflight simulator for the M5Stack StopWatch (ESP32-S3). Explore planets, the Moon, and Voyager 1/2 with gravity, thrusters, and warp travel.

## The voygerM5 Story

In November 2026, a mysteriously Doppler-shifted transmission arrived from the direction of the Magellanic Clouds. NASA traced it to **voygerM5**, an interstellar test craft launched forty years earlier. Months later it reappeared near the Moon, fitted with impossible new engines.

[Read the voygerM5 story](STORY.md) · [日本語版](STORY.ja.md)

## Controls
- Swipe center: change ship attitude (RCS).
- Swipe lower outer rim: throttle (left reverse / 6 o’clock neutral / right forward).
- Hold right button: main thrust.
- Left button: warp destinations; swipe to choose, right button to warp.
- Long press center: toggle HUD.

## Hardware & installation
- ESP32-S3R8, 16MB Flash, 8MB PSRAM, `BOARD_ID=30`.
- Paired libraries: M5Unified 0.3.0 / M5GFX 0.3.0.
- Independent StopWatch startup: `firmware/boards/voygerm5-stopwatch.json` and `firmware/src/main.cpp`; the first LCD frame is drawn directly before PSRAM sprite allocation. No external sample cloning or startup hook.
- Build: `BUILD_R16F8_NO_FLASH.cmd`; guarded APP-only flash: `RUN_R16F8_BUILD_FLASH_APP_ONLY_COM16.cmd`.
- **Never flash at generic offset 0x10000**. The guarded CMD reads the actual partition table and OTA boot selection. The COM16 device reported a single 3 MB `ota_0` at `0x20000`, no `ota_1`, and storage starting at `0x320000`. Only a recognized layout with a validated `ota_0` boot target permits an APP-only write; all other partitions remain untouched.

**Independent startup update:** Windows PlatformIO build succeeded on November 19, 2026 (2,359,413 bytes Flash usage). The old layout check blocked flashing before any changes were made. The revised single-slot flash guard still requires a successful live OTA-data check; the updated app has not yet been flashed or display-tested. The earlier R16 FIX4 displayed successfully on hardware.

[日本語](README.ja.md) · [引継ぎメモ](HANDOFF.ja.md) · [License](LICENSE) · [Asset notices](THIRD_PARTY_NOTICES.md)

## Source code / ソース

- [Browse firmware source](firmware/src/main.cpp) / [ワープ表示タイミング](firmware/include/warp_timing.hpp)
- [Download full R16 FIX8 source archive](source/SV_R16F8_WARP_FX_COM16.zip)
