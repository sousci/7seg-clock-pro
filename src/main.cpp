#include <ArduinoOTA.h>
#include <SPI.h>
#include <WiFi.h>
#include <secrets.h>
#include "config.h"
#include "chime_player.h"
#include "led_control.h"
#include "web_server.h"

namespace {
LedController ledController;
ChimePlayer chimePlayer;

void networkTask(void *) {
  uint32_t lastWifiAttempt = 0, lastNtpSync = 0, appliedGeneration = UINT32_MAX;
  bool otaStarted = false;
  bool apRunning = false;
  bool stationReported = false;
  bool ntpReported = false;
  WiFi.mode(WIFI_AP_STA);
  ArduinoOTA.setHostname("7seg-clock-pro");
  ArduinoOTA.setPassword(OTA_PASSWORD);
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
      lastNtpSync = 0;
    }
    if (WiFi.status() != WL_CONNECTED &&
        (lastWifiAttempt == 0 || now - lastWifiAttempt >= WIFI_RETRY_INTERVAL_MS)) {
      lastWifiAttempt = now;
      if (current.wifiSsid[0] != '\0') WiFi.begin(current.wifiSsid, current.wifiPassword);
    }
    if (WiFi.status() == WL_CONNECTED) {
      if (apRunning) { WiFi.softAPdisconnect(true); apRunning = false; }
      if (!stationReported) {
        Serial.printf("Wi-Fi connected. IP address: %s\n", WiFi.localIP().toString().c_str());
        stationReported = true;
      }
      if (!otaStarted) { ArduinoOTA.begin(); otaStarted = true; }
      ArduinoOTA.handle();
      if (lastNtpSync == 0 || now - lastNtpSync >= NTP_RESYNC_INTERVAL_MS) {
        configTime(current.utcOffsetSeconds, 0, current.ntpServers[0], current.ntpServers[1], current.ntpServers[2]);
        lastNtpSync = now;
      }
      tm synced{};
      if (!ntpReported && getLocalTime(&synced, 10)) {
        Serial.printf("NTP synchronized. IP address: %s\n", WiFi.localIP().toString().c_str());
        ntpReported = true;
      }
    } else if (!apRunning) {
      stationReported = false;
      ntpReported = false;
      WiFi.softAP("7seg-clock-pro");
      apRunning = true;
      Serial.printf("Config AP: %s\n", WiFi.softAPIP().toString().c_str());
    }
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
      FastLED.setBrightness(settings.brightness);
      lastBrightness = settings.brightness;
    }
    const uint32_t tick = millis();
    chimePlayer.update(tick);
    if (tick - lastRender >= 1000) {
      lastRender = tick;
      tm now{};
      if (getLocalTime(&now, 20)) {
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
  startWebServer();
  xTaskCreatePinnedToCore(networkTask, "network", 6144, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(clockTask, "clock", 6144, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelay(portMAX_DELAY); }
