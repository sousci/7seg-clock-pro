#include "oled_display.h"

#include "config.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>

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

void OledDisplay::render(const tm *timeInfo, bool chimePlaying) {
  if (!available_) return;
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
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
