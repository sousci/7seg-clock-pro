#include "led_control.h"

#include "120pixel.h"
#include "config.h"

uint16_t LedController::physicalPixel(uint8_t digit, uint8_t pixel) const {
  return digit < 2 ? digit * PIXELS_PER_DIGIT + pixel : digit * PIXELS_PER_DIGIT + COLON_PIXEL_COUNT + pixel;
}

CRGB LedController::colorFor(uint8_t digit, uint16_t pixel) const {
  const uint8_t mode = static_cast<uint8_t>(settings.ledMode);
  if (mode == static_cast<uint8_t>(LedColorMode::Solid)) return CRGB(settings.solidColor.r, settings.solidColor.g, settings.solidColor.b);
  if (mode == static_cast<uint8_t>(LedColorMode::PerDigit)) {
    const RgbConfig &color = settings.digitColors[digit]; return CRGB(color.r, color.g, color.b);
  }
  if (mode == static_cast<uint8_t>(LedColorMode::Rainbow)) return CHSV(static_cast<uint8_t>(pixel * 255 / LED_COUNT), 255, 255);
  if (mode == static_cast<uint8_t>(LedColorMode::Animated)) return CHSV(static_cast<uint8_t>((millis() * settings.animationSpeed / 32) + pixel * 3), 255, 255);
  return blend(CRGB(settings.digitColors[0].r, settings.digitColors[0].g, settings.digitColors[0].b),
               CRGB(settings.digitColors[3].r, settings.digitColors[3].g, settings.digitColors[3].b),
               static_cast<uint8_t>(pixel * 255 / (LED_COUNT - 1)));
}

CRGB LedController::solidDebugColor() const {
  return CRGB(settings.solidColor.r, settings.solidColor.g, settings.solidColor.b);
}

void LedController::begin() { FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds_, LED_COUNT); FastLED.setBrightness(settings.brightness); FastLED.clear(true); }
void LedController::setBrightness(uint8_t brightness) { FastLED.setBrightness(brightness); }

void LedController::startDebugAllOn(uint32_t durationMs) {
  debugMode_ = 1;
  debugUntil_ = millis() + durationMs;
  fill_solid(leds_, LED_COUNT, solidDebugColor());
  FastLED.show();
}

void LedController::startDebugChase() {
  debugMode_ = 2;
  debugPixel_ = 0;
  debugLastStep_ = 0;
  fill_solid(leds_, LED_COUNT, CRGB::Black);
  FastLED.show();
}

bool LedController::updateDebug(uint32_t now) {
  if (debugMode_ == 0) return false;
  if (debugMode_ == 1) {
    if (static_cast<int32_t>(now - debugUntil_) < 0) return true;
    debugMode_ = 0;
    fill_solid(leds_, LED_COUNT, CRGB::Black);
    FastLED.show();
    return false;
  }
  if (debugMode_ == 2) {
    if (debugPixel_ >= LED_COUNT) {
      debugMode_ = 0;
      fill_solid(leds_, LED_COUNT, CRGB::Black);
      FastLED.show();
      return false;
    }
    if (debugLastStep_ == 0 || now - debugLastStep_ >= 80) {
      debugLastStep_ = now;
      fill_solid(leds_, LED_COUNT, CRGB::Black);
      leds_[debugPixel_++] = solidDebugColor();
      FastLED.show();
    }
    return true;
  }
  debugMode_ = 0;
  return false;
}

void LedController::render(const tm &timeInfo, bool colonOn, bool sleeping) {
  fill_solid(leds_, LED_COUNT, CRGB::Black);
  if (sleeping) { leds_[0] = CRGB(10, 40, 0); FastLED.show(); return; }
  const uint8_t value[DIGIT_COUNT] = {static_cast<uint8_t>(timeInfo.tm_hour / 10), static_cast<uint8_t>(timeInfo.tm_hour % 10), static_cast<uint8_t>(timeInfo.tm_min / 10), static_cast<uint8_t>(timeInfo.tm_min % 10)};
  for (uint8_t digit = 0; digit < DIGIT_COUNT; ++digit) for (uint8_t segment = 0; segment < PIXELS_PER_DIGIT; ++segment) if (digitSegments[value[digit]][segment]) leds_[physicalPixel(digit, segment)] = colorFor(digit, physicalPixel(digit, segment));
  if (colonOn) for (uint8_t pixel : COLON_PIXELS) leds_[pixel] = colorFor(1, pixel);
  FastLED.show();
}

bool isSleepTime(const tm &timeInfo) {
  if (!settings.sleepEnabled) return false;
  const uint16_t now = timeInfo.tm_hour * 60 + timeInfo.tm_min, start = settings.sleepStartHour * 60 + settings.sleepStartMinute, end = settings.sleepEndHour * 60 + settings.sleepEndMinute;
  if (start == end) return false;
  return start < end ? now >= start && now < end : now >= start || now < end;
}
