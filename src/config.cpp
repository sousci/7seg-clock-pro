#include "config.h"

ClockConfig settings = {
    "", "", {"ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp"},
    9 * 60 * 60, LedColorMode::Gradient, {0, 128, 255},
    {{255, 0, 0}, {255, 128, 0}, {0, 255, 0}, {0, 128, 255}},
    LED_BRIGHTNESS_DEFAULT, 32, false, 22, 0, 6, 0, {}};

SemaphoreHandle_t settingsMutex = nullptr;
volatile uint32_t configGeneration = 0;
volatile bool ntpSyncRequested = false;
volatile uint8_t ntpStatus = 0;
