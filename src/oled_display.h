#pragma once

#include <time.h>

class OledDisplay {
 public:
  bool begin();
  void render(const tm *timeInfo, bool chimePlaying);
  void showMessage(const char *line1, const char *line2, const char *line3, uint32_t durationMs);
  bool available() const { return available_; }
 private:
  bool available_ = false;
  char message_[3][22]{};
  uint32_t messageUntil_ = 0;
};
