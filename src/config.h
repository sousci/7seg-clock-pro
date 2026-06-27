#pragma once

#include <Arduino.h>
#include <FastLED.h>

// OLED and speaker values are provisional until chime_machine.ino is supplied.
constexpr uint8_t LED_PIN = 27;
constexpr uint16_t LED_COUNT = 120;
constexpr uint8_t LED_BRIGHTNESS_DEFAULT = 255;
constexpr uint8_t RTC_SDA_PIN = 21;
constexpr uint8_t RTC_SCL_PIN = 22;
constexpr uint8_t RTC_I2C_ADDRESS = 0x51;  // RTC-8564NB (7-bit address)
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t SPEAKER_PIN = 25;

constexpr uint8_t DIGIT_COUNT = 4;
constexpr uint8_t PIXELS_PER_DIGIT = 29;
constexpr uint8_t COLON_PIXEL_COUNT = 4;
constexpr uint8_t COLON_PIXELS[COLON_PIXEL_COUNT] = {58, 59, 60, 61};
constexpr uint8_t MAX_CHIME_SCHEDULES = 24;
constexpr uint8_t MAX_NTP_SERVERS = 3;

constexpr char FIRMWARE_VERSION[] = "1.0.0";
constexpr char ONLINE_FIRMWARE_URL[] =
    "https://github.com/sousci/7seg-clock-pro/releases/download/v1.0.0/firmware.bin";
constexpr char ONLINE_FILESYSTEM_URL[] =
    "https://github.com/sousci/7seg-clock-pro/releases/download/v1.0.0/littlefs.bin";

constexpr uint32_t NTP_RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15UL * 1000UL;
constexpr uint32_t OTA_ENABLE_WINDOW_MS = 5UL * 60UL * 1000UL;

enum class LedColorMode : uint8_t { Solid, PerDigit, Gradient, Rainbow, Animated };
enum class DebugAction : uint8_t { None, LedAllOn, LedChase, Chime, Oled, Rtc, Ntp };

struct RgbConfig { uint8_t r; uint8_t g; uint8_t b; };

struct ChimeSchedule {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t melodyId;
  uint8_t weekdays;  // bit 0: Sunday ... bit 6: Saturday
};

struct ClockConfig {
  char webAdminUser[33];
  char webAdminPassword[65];
  char otaPassword[65];
  char wifiSsid[33];
  char wifiPassword[65];
  char ntpServers[MAX_NTP_SERVERS][64];
  int32_t utcOffsetSeconds;
  LedColorMode ledMode;
  RgbConfig solidColor;
  RgbConfig digitColors[DIGIT_COUNT];
  uint8_t brightness;
  uint8_t animationSpeed;
  bool sleepEnabled;
  uint8_t sleepStartHour;
  uint8_t sleepStartMinute;
  uint8_t sleepEndHour;
  uint8_t sleepEndMinute;
  ChimeSchedule chimes[MAX_CHIME_SCHEDULES];
};

extern ClockConfig settings;
extern SemaphoreHandle_t settingsMutex;
extern volatile uint32_t configGeneration;
extern volatile bool ntpSyncRequested;
extern volatile uint8_t ntpStatus;
extern volatile uint8_t debugActionRequested;
extern volatile bool ntpDebugTestPending;
extern volatile bool otaEnableRequested;
extern volatile bool otaActive;
extern volatile uint32_t otaActiveUntilMs;
extern volatile bool onlineFirmwareUpdateRequested;
extern volatile uint8_t onlineUpdateStatus;
