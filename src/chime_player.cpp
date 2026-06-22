#include "chime_player.h"

namespace {
constexpr uint16_t NOTES[] = {330, 262, 294, 196, 196, 294, 330, 262};
constexpr uint16_t DURATIONS[] = {1000, 1000, 1000, 3000, 1000, 1000, 1000, 3000};
constexpr uint8_t NOTE_COUNT = sizeof(NOTES) / sizeof(NOTES[0]);
constexpr uint16_t REST_MS = 50;
}

void ChimePlayer::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  noTone(pin_);
}

void ChimePlayer::start() {
  if (active_) return;
  noteIndex_ = 0;
  rest_ = false;
  active_ = true;
  tone(pin_, NOTES[noteIndex_]);
  deadline_ = millis() + DURATIONS[noteIndex_];
}

void ChimePlayer::update(uint32_t now) {
  if (!active_ || static_cast<int32_t>(now - deadline_) < 0) return;
  if (!rest_) {
    noTone(pin_);
    rest_ = true;
    deadline_ = now + REST_MS;
    return;
  }
  if (++noteIndex_ >= NOTE_COUNT) { active_ = false; rest_ = false; return; }
  rest_ = false;
  tone(pin_, NOTES[noteIndex_]);
  deadline_ = now + DURATIONS[noteIndex_];
}
