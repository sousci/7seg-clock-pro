#include <ArduinoOTA.h>
#include <WiFi.h>
#include "config.h"
#include "120pixel.h"

namespace {
CRGB leds[LED_COUNT];

uint16_t physicalPixel(uint8_t digit, uint8_t pixel) {
  return digit < 2 ? digit * PIXELS_PER_DIGIT + pixel
                   : digit * PIXELS_PER_DIGIT + COLON_PIXEL_COUNT + pixel;
}

void renderTime(const tm &timeInfo, bool colonOn) {
  const uint8_t value[DIGIT_COUNT] = {
      static_cast<uint8_t>(timeInfo.tm_hour / 10), static_cast<uint8_t>(timeInfo.tm_hour % 10),
      static_cast<uint8_t>(timeInfo.tm_min / 10), static_cast<uint8_t>(timeInfo.tm_min % 10)};
  fill_solid(leds, LED_COUNT, CRGB::Black);
  for (uint8_t digit = 0; digit < DIGIT_COUNT; ++digit) {
    for (uint8_t pixel = 0; pixel < PIXELS_PER_DIGIT; ++pixel) {
      if (!digitSegments[value[digit]][pixel]) continue;
      const uint8_t hue = map(physicalPixel(digit, pixel), 0, LED_COUNT - 1, 64, 128);
      leds[physicalPixel(digit, pixel)] = CHSV(hue, 255, 200);
    }
  }
  if (colonOn) for (uint8_t pixel : COLON_PIXELS) leds[pixel] = CRGB(100, 100, 100);
  FastLED.show();
}

void networkTask(void *) {
  uint32_t lastWifiAttempt = 0, lastNtpSync = 0;
  bool otaStarted = false;
  WiFi.mode(WIFI_STA);
  for (;;) {
    const uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED &&
        (lastWifiAttempt == 0 || now - lastWifiAttempt >= WIFI_RETRY_INTERVAL_MS)) {
      lastWifiAttempt = now;
      if (settings.wifiSsid[0] != '\0') WiFi.begin(settings.wifiSsid, settings.wifiPassword);
    }
    if (WiFi.status() == WL_CONNECTED) {
      if (!otaStarted) { ArduinoOTA.begin(); otaStarted = true; }
      ArduinoOTA.handle();
      if (lastNtpSync == 0 || now - lastNtpSync >= NTP_RESYNC_INTERVAL_MS) {
        configTime(settings.utcOffsetSeconds, 0, settings.ntpServers[0], settings.ntpServers[1], settings.ntpServers[2]);
        lastNtpSync = now;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void clockTask(void *) {
  bool colonOn = false;
  for (;;) {
    tm now{};
    if (getLocalTime(&now, 20)) renderTime(now, colonOn);
    colonOn = !colonOn;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  settingsMutex = xSemaphoreCreateMutex();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(settings.brightness);
  FastLED.clear(true);
  xTaskCreatePinnedToCore(networkTask, "network", 6144, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(clockTask, "clock", 6144, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelay(portMAX_DELAY); }
