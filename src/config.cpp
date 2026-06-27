#include "config.h"

ClockConfig settings = {
    "admin", "admin", "admin",
    "", "", {"ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp"},
    9 * 60 * 60, LedColorMode::Gradient, {0, 128, 255},
    {{255, 0, 0}, {255, 128, 0}, {0, 255, 0}, {0, 128, 255}},
    LED_BRIGHTNESS_DEFAULT, 32, false, 22, 0, 6, 0, {}};

SemaphoreHandle_t settingsMutex = nullptr;
volatile uint32_t configGeneration = 0;
volatile bool ntpSyncRequested = false;
volatile uint8_t ntpStatus = 0;
volatile uint8_t debugActionRequested = static_cast<uint8_t>(DebugAction::None);
volatile bool ntpDebugTestPending = false;
volatile bool otaEnableRequested = false;
volatile bool otaActive = false;
volatile uint32_t otaActiveUntilMs = 0;
volatile bool onlineFirmwareUpdateRequested = false;
volatile uint8_t onlineUpdateStatus = 0;
