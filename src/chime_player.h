#pragma once

#include <Arduino.h>

class ChimePlayer {
 public:
  void begin(uint8_t pin);
  void start();
  void update(uint32_t now);
  bool isPlaying() const { return active_; }

 private:
  uint8_t pin_ = 0;
  uint8_t noteIndex_ = 0;
  uint32_t deadline_ = 0;
  bool active_ = false;
  bool rest_ = false;
};
