#pragma once

#include <FastLED.h>
#include <time.h>

#include "config.h"

class LedController {
 public:
  void begin();
  void render(const tm &timeInfo, bool colonOn, bool sleeping);
  void setBrightness(uint8_t brightness);
  void startDebugAllOn(uint32_t durationMs);
  void startDebugChase();
  bool updateDebug(uint32_t now);

 private:
  CRGB leds_[LED_COUNT]{};
  uint8_t debugMode_ = 0;
  uint16_t debugPixel_ = 0;
  uint32_t debugUntil_ = 0;
  uint32_t debugLastStep_ = 0;
  uint16_t physicalPixel(uint8_t digit, uint8_t pixel) const;
  CRGB colorFor(uint8_t digit, uint16_t pixel) const;
  CRGB solidDebugColor() const;
};

bool isSleepTime(const tm &timeInfo);
