# voygerM5 — Space flight simulator for M5Stack StopWatch

1977年に地球を旅立ち、今も遠い宇宙を進み続けるボイジャー1号。**voygerM5なら、M5Stack StopWatchの小さな画面の中で、あの探査機にもう一度会いに行けます。**

重力、姿勢制御スラスター、ワープを駆使して、惑星や月、ボイジャー1号・2号を訪ねる宇宙航行シミュレータです。

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

[English](README.md) · [開発引継ぎ](HANDOFF.ja.md) · [MIT License](LICENSE)

## Source code / ソース

- [Browse firmware source](firmware/src/main.cpp) / [ワープ表示タイミング](firmware/include/warp_timing.hpp)
- [Download full R16 FIX8 source archive](source/SV_R16F8_WARP_FX_COM16.zip)
