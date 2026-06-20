# NeoPixel 7-Segment Clock Pro

ESP32 と NeoPixel 120 個で構成する大型4桁7セグメント時計のファームウェアです。従来のWi-Fi/NTP時刻表示を基盤に、RTC基準のオフライン運用、Web設定、OTA更新、学校チャイム、OLED状態表示を統合する設備時計として開発しています。

> **開発状況:** FreeRTOSによるタスク分散とFastLEDベースの時刻描画を実装済みです。RTC、LittleFS/Web UI、JSON設定保存、OLED、チャイム再生は実装予定です。実機での動作確認は未実施です。

## 主な機能

- 120ピクセルNeoPixelによる4桁（`HH:MM`）表示
- FastLEDを用いたHSVグラデーション表示
- Core 0 / Core 1 に分離したFreeRTOSタスク構成
- Wi-Fiへの自動再接続とNTP同期
- ArduinoOTAによる無線ファームウェア更新
- 将来拡張用の設定モデル
  - 単色・桁別・グラデーション・虹色・アニメーションのLED色モード
  - 明るさ、アニメーション速度、スリープ時刻
  - NTPサーバー、タイムゾーン、チャイム予定（最大24件）

## ハードウェア要件

| 項目 | 現在の設定 | 備考 |
| --- | --- | --- |
| MCU | ESP32 Dev Module | `esp32dev` 環境 |
| LED | WS2812B/NeoPixel、120個 | GRB、800 kHz |
| LEDデータピン | GPIO 27 | `config.h` で変更可能 |
| RTC | DS3231等のI2C RTC（予定） | SDA GPIO 21、SCL GPIO 22 |
| OLED | I2C OLED（予定） | I2Cアドレス `0x3C`（暫定） |
| スピーカー | PWM/I2S接続（予定） | GPIO 26（暫定） |

OLEDおよびスピーカーのピン・駆動方式は、統合対象の `chime_machine.ino` を受領後に確定します。現時点の値はコンパイル時の既定値であり、実機配線の確定値ではありません。

## アーキテクチャ

ESP32の二つのCPUコアを、ネットワーク処理とリアルタイム表示処理に分離します。NeoPixel、RTC、OLED、スピーカーはすべてCore 1で扱い、同一のハードウェアバスやタイミング依存リソースを複数タスクから同時に操作しません。

| Core | タスク | 周期・方式 | 責務 |
| --- | --- | --- | --- |
| 0 | `networkTask` | 常駐 | Wi-Fi再接続、NTP同期、ArduinoOTA処理 |
| 0 | Async Web Server（予定） | イベント駆動 | 静的UI配信、REST API、設定更新 |
| 1 | `clockTask` | 1秒周期 | 時刻取得、LED描画、OLED更新、チャイム判定・再生 |
| 1 | `loop()` | 待機 | Arduinoランタイム用の待機ループ |

設定値は `ClockConfig` で一元管理し、将来のWeb APIから更新する場合は `settingsMutex` により排他します。設定の永続化はLittleFS上の`/config.json`を予定し、ArduinoJsonでシリアライズします。

## ディレクトリ構成

```text
.
├── platformio.ini             # PlatformIO環境と依存ライブラリ
├── src/
│   ├── main.cpp               # setup/loop、FreeRTOSタスク、現行LED描画
│   ├── config.h               # ハードウェア定数と設定構造体
│   └── 120pixel.h             # 120ピクセル用の数字セグメント対応表
└── data/                      # LittleFSへアップロードするWeb UI（実装予定）
```

今後は `led_control`、`time_manager`、`web_server`、`chime_player` を個別モジュールとして追加します。

## 設定項目

`src/config.h` の `ClockConfig` が設定の正規形です。

| 分類 | 項目 |
| --- | --- |
| ネットワーク | SSID、パスワード、NTPサーバー3件、UTCオフセット |
| LED | 表示モード、単色、桁別色、明るさ、アニメーション速度 |
| スリープ | 有効フラグ、開始・終了時刻 |
| チャイム | 有効フラグ、時刻、メロディID、曜日ビットマスク（最大24件） |

現在はWi-Fi認証情報の既定値を空文字列にしています。Web設定機能が導入されるまでは、テスト時に `settings.wifiSsid` と `settings.wifiPassword` へ値を設定する必要があります。認証情報をGit管理対象のソースコードへ恒久的に記録しないでください。

## 開発環境

- [PlatformIO](https://platformio.org/)
- ESP32 Arduino framework
- C++17相当のPlatformIO標準設定

主要な依存ライブラリは `platformio.ini` で管理します。

- FastLED
- ArduinoJson
- RTClib
- ESPAsyncWebServer

AsyncTCPはESP32向けESPAsyncWebServerの依存関係として解決される想定です。特定の環境で解決されない場合は、`ESP32Async/AsyncTCP` を明示的な依存ライブラリとして追加してください。

## ビルドと書込み

PlatformIO CLIをインストールした環境で、リポジトリのルートから実行します。

```sh
pio run
pio run --target upload
pio device monitor --baud 115200
```

VS CodeのPlatformIO拡張機能を使用する場合は、プロジェクトフォルダを開き、対象環境 `esp32dev` を選択してBuild/Uploadを実行してください。

## 現在の動作

1. Core 1でFastLEDを初期化し、LEDを消灯します。
2. Core 0がWi-Fi接続を開始します。認証情報が未設定なら接続しません。
3. 接続成功後、NTPを設定し、ArduinoOTAを開始します。
4. Core 1がシステム時刻を毎秒読み取り、`HH:MM`を120ピクセルのグラデーションで描画します。

RTC未実装の現段階では、描画時刻はESP32のシステム時刻に依存します。ネットワーク未接続時、またはNTP同期前は有効な時刻を表示できません。RTC導入後はRTCを通常時の時刻源とし、NTPはRTC補正専用に変更します。

## 実装予定

- DS3231の初期化、RTC読取り、NTP同期時のRTC補正、手動日時設定
- LittleFSの初期化と`/config.json`の原子的な保存・読込み
- `data/` 以下の設定GUI、およびJSON REST API
- LEDの全色モード、スリープ中の低輝度黄緑1ピクセル表示
- `chime_machine.ino` 資産のOLED・スピーカー制御の移植
- チャイムの曜日・時刻スケジュール判定と重複再生防止
- 実機配線・電源容量・LED信号品質を含む動作検証

## 注意事項

- 120個のWS2812Bを高輝度で点灯する場合、ESP32のUSB給電だけでは電力が不足する可能性があります。LED用電源を別途用意し、ESP32とGNDを共通化してください。
- 5 V動作のLEDへ3.3 Vロジック信号を接続する構成では、配線長・電源電圧によって信号品質が低下する場合があります。必要に応じてレベルシフタを使用してください。
- OTAおよびWeb設定画面をネットワークへ公開する前に、認証・アクセス制御の要件を定義してください。現段階ではこれらの保護機構は未実装です。
