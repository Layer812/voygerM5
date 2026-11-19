# voygerM5 — Space flight simulator for M5Stack StopWatch

M5Stack StopWatch用の宇宙航行シミュレータです。重力、姿勢制御スラスター、ワープで惑星・月・Voyager 1/2を訪れます。

## voygerM5の物語

2026年11月、マゼラン雲方面から奇妙な電波が届く。NASAが特定した発信元は、40年前に打ち上げられた恒星間亜光速航行試験船 **voygerM5号** だった。数か月後、月の近くに帰還した船には未知の推進装置が増設されていた。

[voygerM5の物語を読む](STORY.ja.md) · [Read in English](STORY.md)

## 操作
- 中央スワイプ：RCSで機首方向を変更
- 下側の外周スワイプ：スロットル（左＝逆噴射、6時＝中立、右＝前進）
- 右ボタン長押し：主スラスター噴射
- 左ボタン：ワープメニュー（スワイプで行き先選択、右ボタンで決定）
- 中央長押し：HUD表示切替

## ビルド・書き込み
Windows / PlatformIO、ESP32-S3R8、Flash 16MB、PSRAM 8MB。M5Unified/M5GFX 0.3.0、`BOARD_ID=30`。

独立起動方式：`firmware/boards/voygerm5-stopwatch.json` と `firmware/src/main.cpp` を使用します。M5Unified初期化後、PSRAM確保より先にLCDへ初回フレームを直接描画します。外部サンプルのクローンや起動フックは不要です。

ビルドのみ：`BUILD_R16F8_NO_FLASH.cmd`  
APP-ONLY書き込み（COMポートを引数指定）：`RUN_R16F8_BUILD_FLASH_APP_ONLY_COM16.cmd COM13` または `... COM16`。CMD名のCOM16は旧称で、書き込み先COM番号は引数で決まります。

書込CMDは実機のパーティション表とOTA起動情報を読み取り専用で照合します。2026-11-19のCOM16実機では `ota_0=0x20000 / 0x300000`（3MB、`ota_1`なし）、`storage=0x320000` でした。別の機体のfactory-only構成（起動APPが1つだけ）もtype/subtype・重複・範囲・既存APPヘッダーを検査して対応します。認識済みのOTA構成では選択中の起動スロットを検証します。`0x10000`はfactory APPとして実機から確認できた場合に限って書込対象となり、オフセットは決め打ちしません。Bootloader・Partition Table・otadata・NVS・Storageは保護します。

**2026-11-19：実機起動成功（ユーザー報告）。** StopWatch独立起動方式でPlatformIOビルド成功（Flash 2,359,413バイト使用）。実機の単一3MB OTA構成に安全書込みチェックを対応させ、最新voygerM5の起動を確認しました。

[English](README.md) · [開発引継ぎ](HANDOFF.ja.md) · [MIT License](LICENSE)

## Source code / ソース

- [Browse firmware source](firmware/src/main.cpp) / [ワープ表示タイミング](firmware/include/warp_timing.hpp)
- [Download full R16 FIX8 source archive](source/SV_R16F8_WARP_FX_COM16.zip)
