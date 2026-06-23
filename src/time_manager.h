#pragma once
#include <RTClib.h>

class TimeManager {
 public:
  bool begin();
  bool now(tm &out);
  bool adjustFromSystemTime();
  bool available() const { return available_; }
 private:
  RTC_PCF8563 rtc_;
  bool available_ = false;
};

extern TimeManager timeManager;
