#pragma once

#include <FastLED.h>
#include <time.h>

#include "config.h"

class LedController {
 public:
  void begin();
  void render(const tm &timeInfo, bool colonOn, bool sleeping);
  void setBrightness(uint8_t brightness);

 private:
  CRGB leds_[LED_COUNT]{};
  uint16_t physicalPixel(uint8_t digit, uint8_t pixel) const;
  CRGB colorFor(uint8_t digit, uint16_t pixel) const;
};

bool isSleepTime(const tm &timeInfo);
