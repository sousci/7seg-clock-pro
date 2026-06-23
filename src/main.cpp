#include <ArduinoOTA.h>
#include <SPI.h>
#include <WiFi.h>
#include <secrets.h>
#include "config.h"
#include "chime_player.h"
#include "led_control.h"
#include "oled_display.h"
#include "time_manager.h"
#include "web_server.h"

namespace {
LedController ledController;
ChimePlayer chimePlayer;
OledDisplay oledDisplay;

void restartConfigAp() {
  WiFi.softAPdisconnect(true);
  vTaskDelay(pdMS_TO_TICKS(100));
  WiFi.softAP("7seg-clock-pro");
  Serial.printf("Config AP ready: %s\n", WiFi.softAPIP().toString().c_str());
}

void networkTask(void *) {
  uint32_t lastWifiAttempt = 0, appliedGeneration = UINT32_MAX;
  uint32_t ntpStartedAt = 0;
  bool stationAttemptActive = false;
  bool ntpWaiting = false;
  WiFi.mode(WIFI_AP_STA);
  restartConfigAp();
  ArduinoOTA.setHostname("7seg-clock-pro");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  for (;;) {
    const uint32_t now = millis();
    ClockConfig current{};
    if (xSemaphoreTake(settingsMutex, pdMS_TO_TICKS(100))) {
      current = settings;
      xSemaphoreGive(settingsMutex);
    }
    if (appliedGeneration != configGeneration) {
      WiFi.disconnect();
      appliedGeneration = configGeneration;
      lastWifiAttempt = 0;
      WiFi.disconnect(false, false);
    }
    if (WiFi.status() != WL_CONNECTED &&
        (ntpSyncRequested || lastWifiAttempt == 0 || now - lastWifiAttempt >= NTP_RESYNC_INTERVAL_MS)) {
      lastWifiAttempt = now;
      ntpSyncRequested = false;
      if (current.wifiSsid[0] != '\0') { WiFi.begin(current.wifiSsid, current.wifiPassword); stationAttemptActive = true; ntpWaiting = false; ntpStatus = 1; }
    }
    if (WiFi.status() == WL_CONNECTED && stationAttemptActive) {
      if (!ntpWaiting) { configTime(current.utcOffsetSeconds, 0, current.ntpServers[0], current.ntpServers[1], current.ntpServers[2]); ntpStartedAt = now; ntpWaiting = true; ntpStatus = 2; }
      tm synced{};
      if (getLocalTime(&synced, 20)) {
        timeManager.adjustFromSystemTime();
        Serial.printf("NTP synchronized. IP address: %s\n", WiFi.localIP().toString().c_str());
        ntpStatus = 3;
        stationAttemptActive = false; ntpWaiting = false; WiFi.disconnect(false, false); restartConfigAp();
      } else if (now - ntpStartedAt >= 30000) {
        Serial.println("NTP synchronization timed out");
        ntpStatus = 4;
        stationAttemptActive = false; ntpWaiting = false; WiFi.disconnect(false, false); restartConfigAp();
      }
    }
    ArduinoOTA.handle();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void clockTask(void *) {
  bool colonOn = false;
  uint8_t lastBrightness = UINT8_MAX;
  uint32_t lastRender = 0;
  int32_t lastMinuteKey = -1;
  for (;;) {
    if (lastBrightness != settings.brightness) {
      ledController.setBrightness(settings.brightness);
      lastBrightness = settings.brightness;
    }
    const uint32_t tick = millis();
    chimePlayer.update(tick);
    if (tick - lastRender >= 1000) {
      lastRender = tick;
      tm now{};
      if (timeManager.now(now)) {
        const int32_t minuteKey = (now.tm_yday * 1440) + (now.tm_hour * 60) + now.tm_min;
        if (minuteKey != lastMinuteKey) {
          lastMinuteKey = minuteKey;
          const uint8_t weekdayMask = 1U << now.tm_wday;
          for (const ChimeSchedule &schedule : settings.chimes) {
            if (schedule.enabled && schedule.hour == now.tm_hour && schedule.minute == now.tm_min &&
                (schedule.weekdays & weekdayMask)) {
              chimePlayer.start();
              break;
            }
          }
        }
        ledController.render(now, colonOn, isSleepTime(now));
        oledDisplay.render(&now, chimePlayer.isPlaying());
        colonOn = !colonOn;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  settingsMutex = xSemaphoreCreateMutex();
  beginConfigStorage();
  // AsyncWebServer uses LwIP immediately; initialize the Wi-Fi/LwIP stack first.
  WiFi.mode(WIFI_AP_STA);
  ledController.begin();
  chimePlayer.begin(SPEAKER_PIN);
  Serial.printf("RTC: %s\n", timeManager.begin() ? "ready" : "not found");
  oledDisplay.begin();
  startWebServer();
  xTaskCreatePinnedToCore(networkTask, "network", 6144, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(clockTask, "clock", 6144, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelay(portMAX_DELAY); }
