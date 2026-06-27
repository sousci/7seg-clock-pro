#include "oled_display.h"

#include "config.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>

namespace {
Adafruit_SSD1306 display(128, 64, &Wire, -1);
}

bool OledDisplay::begin() {
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  Serial.println("I2C scan start");
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C device found: 0x%02X\n", address);
      ++found;
    }
  }
  Serial.printf("I2C scan complete: %u device(s)\n", found);
  Wire.beginTransmission(OLED_I2C_ADDRESS);
  const bool present = Wire.endTransmission() == 0;
  Serial.printf("I2C OLED 0x%02X: %s\n", OLED_I2C_ADDRESS, present ? "found" : "not found");
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Serial.printf("I2C RTC 0x%02X: %s\n", RTC_I2C_ADDRESS, Wire.endTransmission() == 0 ? "found" : "not found");
  if (!present || !display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) return false;
  available_ = true;
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0, 0); display.println("7seg-clock-pro"); display.println("OLED ready"); display.display();
  return true;
}

void OledDisplay::showMessage(const char *line1, const char *line2, const char *line3, uint32_t durationMs) {
  if (!available_) return;
  strlcpy(message_[0], line1 ? line1 : "", sizeof(message_[0]));
  strlcpy(message_[1], line2 ? line2 : "", sizeof(message_[1]));
  strlcpy(message_[2], line3 ? line3 : "", sizeof(message_[2]));
  messageUntil_ = millis() + durationMs;
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0, 0); display.println(message_[0]);
  display.setTextSize(2); display.setCursor(0, 22); display.println(message_[1]);
  display.setTextSize(1); display.setCursor(0, 54); display.print(message_[2]);
  display.display();
}

void OledDisplay::render(const tm *timeInfo, bool chimePlaying) {
  if (!available_) return;
  if (messageUntil_ && static_cast<int32_t>(millis() - messageUntil_) < 0) return;
  messageUntil_ = 0;
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  if (otaActive) {
    const uint32_t now = millis();
    const uint32_t remaining = static_cast<int32_t>(otaActiveUntilMs - now) > 0 ? (otaActiveUntilMs - now) / 1000UL : 0;
    display.setCursor(0, 0); display.print("OTA ENABLED");
    display.setCursor(0, 12); display.print(WiFi.softAPIP());
    display.setTextSize(2); display.setCursor(22, 28);
    display.printf("%02lu:%02lu", remaining / 60UL, remaining % 60UL);
    display.setTextSize(1); display.setCursor(0, 54);
    display.print("Upload to 192.168.4.1");
    display.display();
    return;
  }
  display.setCursor(0, 0);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("STA/NTP sync");
    display.setCursor(0, 12); display.print(WiFi.localIP());
  } else {
    display.print("AP: 7seg-clock-pro");
    display.setCursor(0, 12); display.print(WiFi.softAPIP());
  }
  display.setTextSize(2); display.setCursor(16, 28);
  if (timeInfo) display.printf("%02d:%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  else display.print("--:--:--");
  display.setTextSize(1); display.setCursor(0, 54);
  if (chimePlaying) display.print("CHIME");
  else if (!timeInfo) display.print("RTC ERROR");
  else if (ntpStatus == 1) display.print("NTP: connecting");
  else if (ntpStatus == 2) display.print("NTP: syncing");
  else if (ntpStatus == 3) display.print("NTP: RTC updated");
  else if (ntpStatus == 4) display.print("NTP: failed / RTC");
  else display.print("RTC clock");
  display.display();
}
