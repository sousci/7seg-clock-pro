# NeoPixel 7-Segment Clock Pro

ESP32 DevKit、120個のWS2812B/NeoPixel、RTC、OLED、ブザーを使う設備時計ファームウェアです。通常時はRTCを時刻源にし、設定用アクセスポイントを常時提供します。既設Wi-FiはNTP同期と保守用OTAのために使用します。

## 現在の機能

- 120ピクセルの4桁 `HH:MM` 時計表示
- FastLEDの単色、桁別色、グラデーション、虹色、アニメーション
- 指定時間帯の消灯と黄緑1ピクセル表示
- 最大24件のチャイム予定と非ブロッキング8音メロディ
- RTC-8564NB（I2C `0x51`）を通常時刻源として使用
- SSD1306 128x64 OLED（I2C `0x3C`）への状態表示
- 設定用AP、LittleFS上のWeb設定画面、JSON設定保存
- Web Basic認証、ArduinoOTA、USB/OTA書込み

## 配線

| ESP32 DevKit | 接続先 | 備考 |
|---|---|---|
| GPIO 27 | NeoPixel DIN | LED信号線 |
| GPIO 25 | パッシブブザー入力 | PWMメロディ出力 |
| GPIO 21 | RTC SDA、OLED SDA | I2C共用 |
| GPIO 22 | RTC SCL、OLED SCL | I2C共用 |
| 3V3 | RTC VCC、OLED VCC | I2Cプルアップも3.3Vにする |
| GND | 全モジュールのGND | NeoPixel外部電源とも共通化 |
| 外部5V | NeoPixel VCC | ESP32のUSB/3.3Vから120LEDを給電しない |

I2CアドレスはRTC-8564NBが `0x51`、SSD1306 OLEDが `0x3C` です。ESP32 GPIOは5V非対応のため、I2CのSDA/SCLを5Vへプルアップしないでください。

## 通常動作

1. 起動時にRTCから時刻を読み、NeoPixel時計とOLEDを更新します。
2. 設定用AP `7seg-clock-pro` を起動します。管理端末は常に `http://192.168.4.1` を使用できます。
3. 保存済みWi-Fiがある場合、起動直後と以後1時間ごとに既設Wi-FiへSTA接続を試みます。
4. NTP同期に成功するとRTCを補正し、STA接続を切断してAP常時運用へ戻ります。
5. Wi-FiまたはNTPに失敗してもRTC時計表示を継続します。

AP/STAの切替時は単一Wi-Fi無線のチャンネル変更により、AP接続端末が一時的に再接続を要する場合があります。

## OLED表示

OLEDはAPモード時にAP名と `192.168.4.1`、STA/NTP同期中に既設Wi-Fi側IPを表示します。下段にはRTC時刻またはチャイム状態を表示します。起動時にはI2Cスキャン結果もSerial Monitorへ出力します。

```text
I2C device found: 0x3C
I2C device found: 0x51
RTC: ready
```

## Web設定

設定画面にはWeb Basic認証が必要です。認証情報は `include/secrets.h` にのみ置き、Gitへ追加しません。

```cpp
#pragma once
#define WEB_ADMIN_USER "admin"
#define WEB_ADMIN_PASSWORD "Web管理用パスワード"
#define OTA_PASSWORD "OTA用の別パスワード"
```

設定画面ではWi-Fi、NTP、明るさ、色モード、スリープ、手動時刻、チャイム予定を保存できます。設定値はLittleFSの `/config.json` に保存されます。

`Upload Filesystem Image` はLittleFS全体を書き換えるため、保存済みWi-Fi設定も消去します。実行後はAPからWi-Fiを再設定してください。

## ビルドとUSB書込み

VS CodeのPlatformIOで `esp32dev` 環境を選択します。

```sh
pio run
pio run --target uploadfs
pio run --target upload
```

Serial Monitorは115200 baudです。書込み時はSerial Monitorを閉じます。接続に失敗する場合は、Upload開始時にESP32のBOOTボタンを押し、`Writing at` 表示後に離します。

## OTA書込み

初回はUSB書込みが必要です。Wi-Fi設定後は `esp32dev-ota` 環境でOTA更新できます。

Windowsユーザー環境変数へ次を設定し、VS Codeを再起動してください。

| 変数 | 内容 |
|---|---|
| `CLOCK_OTA_HOST` | OTA対象のIPアドレス。AP経由なら `192.168.4.1` |
| `CLOCK_OTA_PASSWORD` | `OTA_PASSWORD` と同じ値 |

```sh
pio run -e esp32dev-ota --target upload
```

既設Wi-Fi側のOTAを行う場合は、STA接続中のOLED/Serial Monitor表示IPを `CLOCK_OTA_HOST` に指定します。

## 開発上の注意

- `include/secrets.h` は `.gitignore` で除外済みです。強制追加しないでください。
- I2C配線、RTC/OLEDの検出は起動ログで確認します。
- ブザーはパッシブブザー前提です。アクティブブザーではメロディになりません。
- RTCのバックアップ電源を取り付ければ、停電・電源断後も時計を維持できます。
