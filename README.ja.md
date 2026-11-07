# voygerM5 — Space flight simulator for M5Stack StopWatch

M5Stack StopWatch用の宇宙航行シミュレータです。重力、姿勢制御スラスター、ワープで惑星・月・Voyager 1/2を訪れます。

## ストーリー ― 宛先、間違ってませんか？

2026年11月、マゼラン雲方面から奇妙な電波が届く。NASAが特定した発信元は、40年前に打ち上げられた片道の恒星間亜光速航行試験船 **voygerM5号** だった。数か月後、月の近くに帰還した船には未知の推進装置が増設されていた。そして人々は、船体に直接書き込まれた返事に気づく。

**「宛先、間違ってませんか？」**

[起承転結の物語を読む（日本語）](STORY.ja.md) · [English](STORY.md)

## 操作
- 中央スワイプ：RCSで機首方向を変更
- 下側の外周スワイプ：スロットル（左＝逆噴射、6時＝中立、右＝前進）
- 右ボタン長押し：主スラスター噴射
- 左ボタン：ワープメニュー（スワイプで行き先選択、右ボタンで決定）
- 左ボタンのダブルクリック：機首方向へ1.125光年ワープ（自動方位補正なし、長めのワープ演出と振動）
- 中央長押し：HUD表示切替

## ビルド・書き込み
Windows / PlatformIO、ESP32-S3R8、Flash 16MB、PSRAM 8MB。M5Unified/M5GFX 0.3.0、`BOARD_ID=30`。

ビルドのみ：`BUILD_R16F8_NO_FLASH.cmd`  
COM16へAPPのみ：`RUN_R16F8_BUILD_FLASH_APP_ONLY_COM16.cmd`

書込CMDはパーティションと起動OTAスロットを読み取り専用で照合します。今回の実機で確認した`ota_0`は`0x20000`です。一般的な`0x10000`へは書き込みません。Bootloader・Partition Table・otadata・NVS・Storageを保護します。

**R16 FIX8は実機未確認の開発候補**です。ホストテストPASS。R16 FIX4は実機画面表示に成功しています。

[English](README.md) · [開発引継ぎ](HANDOFF.ja.md) · [MIT License](LICENSE)

## Source code / ソース

- [Browse firmware source](firmware/src/main.cpp) / [ワープ表示タイミング](firmware/include/warp_timing.hpp)
- [Download full R16 FIX8 source archive](source/SV_R16F8_WARP_FX_COM16.zip)
