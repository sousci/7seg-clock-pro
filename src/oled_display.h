#pragma once

#include <time.h>

class OledDisplay {
 public:
  bool begin();
  void render(const tm *timeInfo, bool chimePlaying);
  bool available() const { return available_; }
 private:
  bool available_ = false;
};
