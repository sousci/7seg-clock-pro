#pragma once

#include <Arduino.h>
#include <FastLED.h>

// OLED and speaker values are provisional until chime_machine.ino is supplied.
constexpr uint8_t LED_PIN = 27;
constexpr uint16_t LED_COUNT = 120;
constexpr uint8_t LED_BRIGHTNESS_DEFAULT = 96;
constexpr uint8_t RTC_SDA_PIN = 21;
constexpr uint8_t RTC_SCL_PIN = 22;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t SPEAKER_PIN = 26;

constexpr uint8_t DIGIT_COUNT = 4;
constexpr uint8_t PIXELS_PER_DIGIT = 29;
constexpr uint8_t COLON_PIXEL_COUNT = 4;
constexpr uint8_t COLON_PIXELS[COLON_PIXEL_COUNT] = {58, 59, 60, 61};
constexpr uint8_t MAX_CHIME_SCHEDULES = 24;
constexpr uint8_t MAX_NTP_SERVERS = 3;

constexpr uint32_t NTP_RESYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15UL * 1000UL;

enum class LedColorMode : uint8_t { Solid, PerDigit, Gradient, Rainbow, Animated };

struct RgbConfig { uint8_t r; uint8_t g; uint8_t b; };

struct ChimeSchedule {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t melodyId;
  uint8_t weekdays;  // bit 0: Sunday ... bit 6: Saturday
};

struct ClockConfig {
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

inline ClockConfig settings = {
    "", "", {"ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp"},
    9 * 60 * 60, LedColorMode::Gradient, {0, 128, 255},
    {{255, 0, 0}, {255, 128, 0}, {0, 255, 0}, {0, 128, 255}},
    LED_BRIGHTNESS_DEFAULT, 32, false, 22, 0, 6, 0, {}};

inline SemaphoreHandle_t settingsMutex = nullptr;
