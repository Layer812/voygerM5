# STAR VOYAGER — M5Stack StopWatch

Spaceflight simulator for the M5Stack StopWatch (ESP32-S3). Explore planets, the Moon, and Voyager 1/2 with gravity, thrusters, and warp travel.

## Controls
- Swipe center: change ship attitude (RCS).
- Swipe lower outer rim: throttle (left reverse / 6 o’clock neutral / right forward).
- Hold right button: main thrust.
- Left button: warp destinations; swipe to choose, right button to warp.
- Long press center: toggle HUD.

## Hardware & installation
- ESP32-S3R8, 16MB Flash, 8MB PSRAM, `BOARD_ID=30`.
- Paired libraries: M5Unified 0.3.0 / M5GFX 0.3.0.
- Build: `BUILD_R16F8_NO_FLASH.cmd`; guarded APP-only flash: `RUN_R16F8_BUILD_FLASH_APP_ONLY_COM16.cmd`.
- **Never flash at generic offset 0x10000**. The guarded CMD checks the real partition table and OTA boot slot, then writes only bootable `ota_0` at `0x20000` on the confirmed device.

**R16 FIX8:** warp animation frame-pacing recovery, arrival positioning, short 1.8-second exit glow, no destination markers or names in flight. Host tests PASS; Windows PlatformIO build and R16 FIX8 device test pending. R16 FIX4 displayed successfully on hardware.

[日本語](README.ja.md) · [引継ぎメモ](HANDOFF.ja.md) · [License](LICENSE) · [Asset notices](THIRD_PARTY_NOTICES.md)
