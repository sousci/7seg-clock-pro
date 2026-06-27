# NeoPixel 7-Segment Clock Pro

ESP32 DevKit、WS2812B/NeoPixel大型7セグメント表示、RTC、OLED、ブザーを組み合わせた設備時計ファームウェアです。

通常時はRTCを時刻源として動作し、設定用アクセスポイントを常時提供します。既設Wi-FiはNTPによるRTC補正時のみ一時的に使用します。OTA更新はセキュリティ上、Web設定画面から明示的に有効化した後の5分間だけ受け付けます。

## 現在の主な機能

- 120ピクセル構成のNeoPixel 4桁 `HH:MM` 表示
- FastLEDによる単色、桁別色、グラデーション、虹色、アニメーション表示
- 指定時間帯のスリープ表示
  - 通常LED表示をOFF
  - 通電確認用として1ピクセルのみ低輝度黄緑色で点灯
- RTC-8564NB / PCF8563互換RTCを通常時刻源として使用
- NTP同期によるRTC補正
- SSD1306 128x64 OLEDへの状態表示
- ブザーによるチャイム再生
- 最大24件のチャイムスケジュール
- LittleFS上のWeb設定画面
- NVSへのJSON形式の設定保存
- Web Basic認証
- ArduinoOTAによるファームウェア更新
- Webデバッグ機能
  - 全LED点灯
  - 1個ずつ順番にLED点灯
  - チャイムテスト
  - OLEDテスト
  - RTCテスト
  - NTPアクセステスト
  - OTAを5分間有効化

## ハードウェア構成

| ESP32 DevKit | 接続先 | 備考 |
|---|---|---|
| GPIO 27 | NeoPixel DIN | LEDデータ信号 |
| GPIO 25 | ブザー | `tone()` によるチャイム出力 |
| GPIO 21 | RTC SDA / OLED SDA | I2C共用 |
| GPIO 22 | RTC SCL / OLED SCL | I2C共用 |
| 3V3 | RTC VCC / OLED VCC | I2Cプルアップは3.3V系にする |
| GND | 全モジュールGND | NeoPixel外部電源GNDとも共通化 |
| 外部5V | NeoPixel VCC | ESP32の3.3VからLEDへ給電しない |

I2Cアドレス：

| デバイス | アドレス |
|---|---|
| SSD1306 OLED | `0x3C` |
| RTC-8564NB / PCF8563 | `0x51` |

ESP32のGPIOは5Vトレラントではありません。I2CのSDA/SCLを5Vへプルアップしないでください。

NeoPixelは消費電流が大きいため、LEDの先頭だけでなく中間・末端にも5V/GNDを追加給電する構成を推奨します。

## 起動時・通常時の動作

1. ESP32起動
2. NVSから設定を読み込み
3. RTCを初期化
4. OLEDとI2Cバスを初期化
5. 設定用AP `7seg-clock-pro` を起動
6. Web設定画面を `http://192.168.4.1/` で提供
7. RTC時刻を基準にNeoPixel時計表示とOLED表示を更新
8. 保存済みWi-Fi設定がある場合、NTP同期タイミングで既設Wi-Fiへ一時接続
9. NTP同期に成功した場合、RTCをJST時刻で補正
10. NTP処理後はSTA接続を切断し、AP常時運用へ戻る

Wi-FiやNTPに失敗しても、RTCが動作していれば時計表示は継続します。

## OLED表示

OLEDには現在状態を表示します。

- APモード時
  - AP名
  - AP側IP `192.168.4.1`
- NTP同期中
  - 既設Wi-Fi側IP
  - NTP同期状態
- 通常時
  - RTC時刻
  - RTC/NTP/チャイム状態
- テスト時
  - RTCテスト結果
  - NTPテスト結果
  - OTA有効化状態

起動時にはSerial MonitorへI2Cスキャン結果も出力します。

正常例：

```text
RTC: ready
I2C scan start
I2C device found: 0x3C
I2C device found: 0x51
I2C OLED 0x3C: found
I2C RTC 0x51: found
Config AP ready: 192.168.4.1
```

## Web設定画面

PCまたはスマートフォンをESP32のAPへ接続し、ブラウザで開きます。

```text
http://192.168.4.1/
```

Web Basic認証が必要です。初期値はユーザー `admin`、パスワード `admin` です。運用前にWeb設定画面から変更してください。

設定可能項目：

- 既設Wi-Fi SSID / パスワード
- Web管理ユーザー / パスワード
- OTAパスワード
- NTPサーバー
- 手動時刻設定
- NTP即時同期
- LED明るさ
- LED色モード
- 単色設定
- 桁別色設定
- アニメーション速度
- スリープ時間帯
- チャイムスケジュール

設定値はNVSに保存されます。LittleFSはWeb UIファイル配信用にのみ使用します。

## デバッグ機能

Web設定画面の最下部にある「デバッグ」ボタンを押すと、デバッグ用ボタンが表示されます。

| ボタン | 動作 |
|---|---|
| 全LED点灯 | 現在の単色設定で5秒間全LEDを点灯 |
| 1つずつ順番に点灯 | LED 0から順番に単色点灯 |
| チャイムテスト | チャイムを即時再生 |
| OLEDテスト | OLEDへテスト結果を5秒表示 |
| RTCテスト | RTC読出し結果をOLEDへ5秒表示 |
| NTPアクセステスト | NTP同期を要求し、結果をOLEDへ表示 |
| OTAを5分間有効化 | OTA待受を5分間だけ有効化 |

## 認証情報とNVS

本構成では `include/secrets.h` は不要です。Web管理パスワード、OTAパスワード、Wi-Fi認証情報、LED設定、チャイム設定はNVSに保存されます。

初期認証情報：

| 項目 | 初期値 |
|---|---|
| Web管理ユーザー | `admin` |
| Web管理パスワード | `admin` |
| OTAパスワード | `admin` |

注意：

- 初回ログイン後、Web管理パスワードとOTAパスワードを必ず変更してください
- Web管理パスワードとOTAパスワードは別の値を推奨します
- OTAパスワードが漏えいした場合、不正ファームウェアを書き込まれる可能性があります
- 認証情報はファームウェアやLittleFSイメージには含まれないため、公開用 `firmware.bin` / `littlefs.bin` を作成しやすい構成です

## PlatformIO環境

`platformio.ini` には2つの環境があります。

| 環境 | 用途 |
|---|---|
| `esp32dev` | USBシリアル書き込み用 |
| `esp32dev-ota` | Wi-Fi OTA書き込み用 |

書き込む対象は2種類あります。

| 対象 | PlatformIOターゲット | 内容 |
|---|---|---|
| ファームウェア | `upload` | `src/`, `include/` から生成される動作プログラム |
| LittleFS | `uploadfs` | `data/` 配下のWeb UIファイル |

`data/index.html`、`data/style.css`、`data/script.js` を変更した場合は `uploadfs` が必要です。  
`src/` や `include/` を変更した場合は `upload` が必要です。

NVSに保存された設定は、通常の `upload` や `uploadfs` では消えません。

## Windows環境構築

### 1. 必要ソフト

- Visual Studio Code
- PlatformIO IDE拡張
- CP210x USB-UARTドライバ
  - デバイスマネージャで `CP2102N USB to UART Bridge Controller` が未認識の場合に必要

### 2. VSCodeでプロジェクトを開く

```text
C:\Users\admin\workspace\7seg-clock-pro
```

をVSCodeで開きます。

### 3. 初回認証情報

初期状態ではWeb管理ユーザー/パスワード、OTAパスワードはいずれも `admin` です。初回書き込み後、Web設定画面から必ず変更してください。

### 4. シリアルモニタ設定

`platformio.ini` で以下を設定済みです。

```ini
monitor_speed = 115200
```

Serial Monitorは115200 baudで開いてください。

## 初回USB書き込み

最初の1回、またはOTAが使えない状態ではUSBで書き込みます。

ESP32をUSB接続し、Serial Monitorを閉じてから実行します。

ファームウェア：

```powershell
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev --target upload
```

Web UI / LittleFS：

```powershell
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev --target uploadfs
```

初回セットアップでは基本的に両方実行してください。

```powershell
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev --target upload
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev --target uploadfs
```

設定はNVSに保存されるため、`uploadfs` でWeb UIを更新してもWi-Fi設定やパスワード設定は維持されます。

## OTA更新の考え方

OTAは常時有効ではありません。Web設定画面のデバッグ機能から「OTAを5分間有効化」を押した後、5分間だけOTA更新できます。

推奨するOTA経路はESP32自身のAP経由です。

```text
PC → Wi-Fi接続 → 7seg-clock-pro
PC → http://192.168.4.1/
PC → OTA upload to 192.168.4.1
```

既設Wi-FiのIPアドレスを調べる必要がなく、現場でルーターに依存しにくいためです。

## Windowsで任意タイミングにOTA更新する手順

### 1. PCをESP32のAPへ接続

WindowsのWi-Fi一覧から以下へ接続します。

```text
7seg-clock-pro
```

### 2. Web設定画面を開く

```text
http://192.168.4.1/
```

Web Basic認証でログインします。

### 3. OTAを5分間有効化

Web画面最下部：

```text
デバッグ → OTAを5分間有効化
```

を押します。

OLEDに以下のような表示が出ます。

```text
OTA ENABLED
192.168.4.1
04:59
Upload to 192.168.4.1
```

OTA有効中は5分間の残り時間を `mm:ss` 形式で表示し続けます。

### 4. PowerShellでOTA用環境変数を設定

同じPowerShellセッションで実行してください。

```powershell
$env:CLOCK_OTA_HOST="192.168.4.1"
$env:CLOCK_OTA_PASSWORD="Web設定画面で設定したOTAパスワード"
```

確認：

```powershell
echo $env:CLOCK_OTA_HOST
echo $env:CLOCK_OTA_PASSWORD
```

`CLOCK_OTA_HOST` が `192.168.4.1`、`CLOCK_OTA_PASSWORD` がOTAパスワードになっていることを確認します。

### 5. ファームウェアをOTA更新

```powershell
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev-ota --target upload
```

### 6. Web UIを変更した場合のみLittleFSもOTA更新

`data/` 配下を変更した場合に実行します。

```powershell
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev-ota --target uploadfs
```

ファームとWeb UIの両方を更新する場合、作業しやすい順番は以下です。

```powershell
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev-ota --target uploadfs
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev-ota --target upload
```

ファーム更新後はESP32が再起動するため、`uploadfs` を先に行う運用を推奨します。

## OTAでよくあるエラー

### `Host password Not Found`

例：

```text
'esp_ip': 'password'
'auth': ''
Sending invitation to password failed
Host password Not Found
```

原因：

- `CLOCK_OTA_HOST` にIPアドレスではなくパスワード等が入っている
- `CLOCK_OTA_PASSWORD` が空
- VSCodeのタスクにPowerShellの環境変数が渡っていない

対処：

```powershell
$env:CLOCK_OTA_HOST="192.168.4.1"
$env:CLOCK_OTA_PASSWORD="正しいOTAパスワード"
C:\Users\admin\.platformio\penv\Scripts\platformio.exe run --environment esp32dev-ota --target upload
```

VSCodeのサイドバー操作でうまくいかない場合は、PowerShellに直接コマンドを入力してください。

### OTA接続できない

確認項目：

- PCが `7seg-clock-pro` APへ接続されている
- `http://192.168.4.1/` が開ける
- Web画面で「OTAを5分間有効化」を押している
- 5分以内にアップロードしている
- OTAパスワードが一致している
- Windowsファイアウォールが通信を遮断していない

## オンラインアップデートへの対応方針

現在の構成では、機器固有の認証情報をファームウェアやLittleFSへ埋め込まないため、GitHub Releases等に公開した `firmware.bin` と `littlefs.bin` を配布しやすくなっています。

オンライン自己更新を追加する場合は、以下の構成を前提にします。

```text
GitHub Releases
└─ vX.Y.Z
   ├── firmware.bin
   ├── littlefs.bin
   └── manifest.json
```

`manifest.json` にはバージョン、URL、SHA-256等を含め、ESP32側は既設Wi-Fiへ一時接続して取得・検証・更新します。この自己更新機能は本README時点では未実装です。

## VSCode / PlatformIOのタスク表示について

PlatformIO拡張では似たタスク名が複数表示されます。運用上は以下だけ使えば十分です。

USB：

```text
esp32dev → Upload
esp32dev → Upload Filesystem Image
```

OTA：

```text
esp32dev-ota → Upload
esp32dev-ota → Upload Filesystem Image
```

`Upload Filesystem Image OTA` という表示が出る場合がありますが、このプロジェクトでは `esp32dev-ota` 環境の `Upload Filesystem Image` がOTA書き込みになります。混乱を避けるため、上記4つだけを使う運用にしてください。

## 開発時の注意

- 認証情報はNVSに保存されます。公開用ファームウェアへ個別パスワードを埋め込まないでください
- Serial Monitorを開いたままだとUSB書き込みに失敗することがあります
- USB書き込み時に自動リセットできない場合は、ESP32のBOOTボタンを押しながら書き込み開始し、`Writing at...` 表示後に離します
- OLED/RTCが見えない場合は起動ログのI2Cスキャン結果を確認します
- NeoPixelが途中で消える場合は、電圧降下だけでなくDIN/DOUTの向き、途中のはんだ不良、マッピング定義も確認します
- `uploadfs` はWeb UIのみを更新します。NVS設定は維持されます

## 現在の代表的なファイル構成

```text
7seg-clock-pro/
├── data/
│   ├── index.html       # Web設定画面
│   ├── script.js        # Web API呼び出し
│   └── style.css        # Web UIスタイル
├── src/
│   ├── config.h/.cpp    # ピン、設定構造体、共有状態
│   ├── led_control.*    # FastLED表示制御
│   ├── time_manager.*   # RTC/NTP時刻管理
│   ├── oled_display.*   # OLED表示
│   ├── chime_player.*   # ブザーチャイム
│   ├── web_server.*     # Web UI/API/LittleFS
│   └── main.cpp         # FreeRTOSタスク、起動処理
└── platformio.ini
```
