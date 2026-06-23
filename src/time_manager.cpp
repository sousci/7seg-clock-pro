#include "time_manager.h"
#include <time.h>

TimeManager timeManager;

bool TimeManager::begin() { available_ = rtc_.begin(); return available_; }
bool TimeManager::now(tm &out) {
  if (!available_) return false;
  const DateTime value = rtc_.now();
  out = {};
  out.tm_year = value.year() - 1900; out.tm_mon = value.month() - 1; out.tm_mday = value.day();
  out.tm_hour = value.hour(); out.tm_min = value.minute(); out.tm_sec = value.second();
  mktime(&out);
  return true;
}
bool TimeManager::adjustFromSystemTime() {
  const time_t epoch = time(nullptr);
  if (epoch < 1700000000 || !available_) return false;
  tm local{};
  if (!getLocalTime(&local, 1000)) return false;
  rtc_.adjust(DateTime(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                       local.tm_hour, local.tm_min, local.tm_sec));
  return true;
}
