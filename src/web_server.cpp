#include "web_server.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <sys/time.h>

#include "config.h"
#include "time_manager.h"

namespace {
constexpr char CONFIG_PATH[] = "/config.json";
constexpr char NVS_NAMESPACE[] = "clock";
constexpr char NVS_CONFIG_KEY[] = "config";
AsyncWebServer server(80);
Preferences preferences;

bool authenticate(AsyncWebServerRequest *request) {
  const char *user = settings.webAdminUser[0] ? settings.webAdminUser : "admin";
  const char *password = settings.webAdminPassword[0] ? settings.webAdminPassword : "admin";
  if (request->authenticate(user, password)) return true;
  request->requestAuthentication();
  return false;
}

void copyString(const JsonObjectConst &object, const char *key, char *target, size_t size,
                bool allowEmpty = true) {
  if (!object[key].is<const char *>()) return;
  const char *value = object[key];
  if (allowEmpty || value[0] != '\0') strlcpy(target, value, size);
}

void writeConfig(JsonDocument &doc, bool includePassword) {
  JsonObject root = doc.to<JsonObject>();
  root["webAdminUser"] = settings.webAdminUser;
  if (includePassword) {
    root["webAdminPassword"] = settings.webAdminPassword;
    root["otaPassword"] = settings.otaPassword;
  }
  root["webAdminPasswordSet"] = settings.webAdminPassword[0] != '\0';
  root["otaPasswordSet"] = settings.otaPassword[0] != '\0';
  root["wifiSsid"] = settings.wifiSsid;
  if (includePassword) root["wifiPassword"] = settings.wifiPassword;
  JsonArray ntp = root["ntpServers"].to<JsonArray>();
  for (uint8_t i = 0; i < MAX_NTP_SERVERS; ++i) ntp.add(settings.ntpServers[i]);
  root["utcOffsetSeconds"] = settings.utcOffsetSeconds;
  root["brightness"] = settings.brightness;
  root["ledMode"] = static_cast<uint8_t>(settings.ledMode);
  JsonObject solid = root["solidColor"].to<JsonObject>();
  solid["r"] = settings.solidColor.r; solid["g"] = settings.solidColor.g; solid["b"] = settings.solidColor.b;
  JsonArray colors = root["digitColors"].to<JsonArray>();
  for (const RgbConfig &color : settings.digitColors) {
    JsonObject item = colors.add<JsonObject>();
    item["r"] = color.r; item["g"] = color.g; item["b"] = color.b;
  }
  root["animationSpeed"] = settings.animationSpeed;
  root["sleepEnabled"] = settings.sleepEnabled;
  root["sleepStartHour"] = settings.sleepStartHour;
  root["sleepStartMinute"] = settings.sleepStartMinute;
  root["sleepEndHour"] = settings.sleepEndHour;
  root["sleepEndMinute"] = settings.sleepEndMinute;
  JsonArray chimes = root["chimes"].to<JsonArray>();
  for (const ChimeSchedule &schedule : settings.chimes) {
    JsonObject item = chimes.add<JsonObject>();
    item["enabled"] = schedule.enabled;
    item["hour"] = schedule.hour;
    item["minute"] = schedule.minute;
    item["melodyId"] = schedule.melodyId;
    item["weekdays"] = schedule.weekdays;
  }
}

void applyConfig(const JsonObjectConst &root) {
  copyString(root, "webAdminUser", settings.webAdminUser, sizeof(settings.webAdminUser), false);
  copyString(root, "webAdminPassword", settings.webAdminPassword, sizeof(settings.webAdminPassword), false);
  copyString(root, "otaPassword", settings.otaPassword, sizeof(settings.otaPassword), false);
  copyString(root, "wifiSsid", settings.wifiSsid, sizeof(settings.wifiSsid));
  copyString(root, "wifiPassword", settings.wifiPassword, sizeof(settings.wifiPassword), false);
  if (root["ntpServers"].is<JsonArrayConst>()) {
    JsonArrayConst ntp = root["ntpServers"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < MAX_NTP_SERVERS && i < ntp.size(); ++i) {
      if (ntp[i].is<const char *>()) strlcpy(settings.ntpServers[i], ntp[i], sizeof(settings.ntpServers[i]));
    }
  }
  if (root["utcOffsetSeconds"].is<int32_t>()) settings.utcOffsetSeconds = root["utcOffsetSeconds"];
  if (root["brightness"].is<uint8_t>()) settings.brightness = root["brightness"];
  if (root["ledMode"].is<uint8_t>()) settings.ledMode = static_cast<LedColorMode>(root["ledMode"].as<uint8_t>());
  if (root["solidColor"].is<JsonObjectConst>()) {
    JsonObjectConst color = root["solidColor"].as<JsonObjectConst>();
    if (color["r"].is<uint8_t>()) settings.solidColor.r = color["r"];
    if (color["g"].is<uint8_t>()) settings.solidColor.g = color["g"];
    if (color["b"].is<uint8_t>()) settings.solidColor.b = color["b"];
  }
  if (root["digitColors"].is<JsonArrayConst>()) {
    JsonArrayConst colors = root["digitColors"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < DIGIT_COUNT && i < colors.size(); ++i) {
      JsonObjectConst color = colors[i].as<JsonObjectConst>();
      if (color["r"].is<uint8_t>()) settings.digitColors[i].r = color["r"];
      if (color["g"].is<uint8_t>()) settings.digitColors[i].g = color["g"];
      if (color["b"].is<uint8_t>()) settings.digitColors[i].b = color["b"];
    }
  }
  if (root["animationSpeed"].is<uint8_t>()) settings.animationSpeed = root["animationSpeed"];
  if (root["sleepEnabled"].is<bool>()) settings.sleepEnabled = root["sleepEnabled"];
  if (root["sleepStartHour"].is<uint8_t>()) settings.sleepStartHour = root["sleepStartHour"];
  if (root["sleepStartMinute"].is<uint8_t>()) settings.sleepStartMinute = root["sleepStartMinute"];
  if (root["sleepEndHour"].is<uint8_t>()) settings.sleepEndHour = root["sleepEndHour"];
  if (root["sleepEndMinute"].is<uint8_t>()) settings.sleepEndMinute = root["sleepEndMinute"];
  if (root["chimes"].is<JsonArrayConst>()) {
    JsonArrayConst chimes = root["chimes"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < MAX_CHIME_SCHEDULES; ++i) settings.chimes[i] = {};
    for (uint8_t i = 0; i < MAX_CHIME_SCHEDULES && i < chimes.size(); ++i) {
      JsonObjectConst item = chimes[i].as<JsonObjectConst>();
      settings.chimes[i].enabled = item["enabled"] | false;
      const uint8_t hour = item["hour"] | 0;
      const uint8_t minute = item["minute"] | 0;
      settings.chimes[i].hour = hour > 23 ? 23 : hour;
      settings.chimes[i].minute = minute > 59 ? 59 : minute;
      settings.chimes[i].melodyId = item["melodyId"] | 0;
      settings.chimes[i].weekdays = item["weekdays"] | 0;
    }
  }
}
}  // namespace

bool beginConfigStorage() {
  if (!LittleFS.begin(true)) return false;
  if (!preferences.begin(NVS_NAMESPACE, false)) return false;
  const String stored = preferences.getString(NVS_CONFIG_KEY, "");
  if (stored.length() > 0) {
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, stored);
    if (error || !doc.is<JsonObject>()) return false;
    applyConfig(doc.as<JsonObjectConst>());
    return true;
  }

  // Migration path for units that still have the previous LittleFS /config.json.
  if (!LittleFS.exists(CONFIG_PATH)) return true;
  File file = LittleFS.open(CONFIG_PATH, "r");
  if (!file) return true;
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error || !doc.is<JsonObject>()) return false;
  applyConfig(doc.as<JsonObjectConst>());
  return saveConfig();
}

bool saveConfig() {
  JsonDocument doc;
  writeConfig(doc, true);
  String body;
  if (serializeJson(doc, body) == 0) return false;
  return preferences.putString(NVS_CONFIG_KEY, body) > 0;
}

void startWebServer() {
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    JsonDocument doc;
    doc["hostname"] = "7seg-clock-pro";
    doc["stationConnected"] = WiFi.status() == WL_CONNECTED;
    doc["stationIp"] = WiFi.localIP().toString();
    doc["apActive"] = WiFi.getMode() & WIFI_AP;
    doc["apIp"] = WiFi.softAPIP().toString();
    doc["timeValid"] = time(nullptr) > 1700000000;
    doc["epoch"] = time(nullptr);
    doc["otaActive"] = otaActive;
    uint32_t otaRemaining = 0;
    if (otaActive) {
      const uint32_t now = millis();
      otaRemaining = static_cast<int32_t>(otaActiveUntilMs - now) > 0 ? (otaActiveUntilMs - now) / 1000UL : 0;
    }
    doc["otaRemainingSeconds"] = otaRemaining;
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    JsonDocument doc;
    if (xSemaphoreTake(settingsMutex, pdMS_TO_TICKS(100))) {
      writeConfig(doc, false);
      xSemaphoreGive(settingsMutex);
    }
    String body;
    serializeJson(doc, body);
    request->send(200, "application/json", body);
  });

  server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *) {}, nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    constexpr size_t MAX_CONFIG_BODY = 4096;
    if (!authenticate(request)) return;
    if (total > MAX_CONFIG_BODY) {
      if (index == 0) request->send(413, "application/json", "{\"error\":\"payload too large\"}");
      return;
    }
    if (index == 0) request->_tempObject = malloc(total + 1);
    if (!request->_tempObject) {
      if (index == 0) request->send(500, "application/json", "{\"error\":\"out of memory\"}");
      return;
    }
    memcpy(static_cast<uint8_t *>(request->_tempObject) + index, data, len);
    if (index + len != total) return;

    static_cast<char *>(request->_tempObject)[total] = '\0';
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, static_cast<char *>(request->_tempObject));
    free(request->_tempObject);
    request->_tempObject = nullptr;
    if (error || !doc.is<JsonObject>()) {
      request->send(400, "application/json", "{\"error\":\"invalid JSON\"}"); return;
    }
    if (!xSemaphoreTake(settingsMutex, pdMS_TO_TICKS(500))) {
      request->send(503, "application/json", "{\"error\":\"settings busy\"}"); return;
    }
    applyConfig(doc.as<JsonObjectConst>());
    const bool saved = saveConfig();
    xSemaphoreGive(settingsMutex);
    if (!saved) { request->send(500, "application/json", "{\"error\":\"save failed\"}"); return; }
    ++configGeneration;
    request->send(200, "application/json", "{\"saved\":true}");
  });

  server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    if (!request->hasParam("epoch", true)) { request->send(400, "application/json", "{\"error\":\"epoch required\"}"); return; }
    const time_t epoch = static_cast<time_t>(request->getParam("epoch", true)->value().toInt());
    if (epoch < 1700000000) { request->send(400, "application/json", "{\"error\":\"invalid epoch\"}"); return; }
    timeval value{epoch, 0};
    settimeofday(&value, nullptr);
    timeManager.adjustFromSystemTime();
    request->send(200, "application/json", "{\"saved\":true}");
  });

  server.on("/api/ntp-sync", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    ntpSyncRequested = true;
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/led-all", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    debugActionRequested = static_cast<uint8_t>(DebugAction::LedAllOn);
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/led-chase", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    debugActionRequested = static_cast<uint8_t>(DebugAction::LedChase);
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/chime", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    debugActionRequested = static_cast<uint8_t>(DebugAction::Chime);
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/oled", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    debugActionRequested = static_cast<uint8_t>(DebugAction::Oled);
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/rtc", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    debugActionRequested = static_cast<uint8_t>(DebugAction::Rtc);
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/ntp", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    ntpDebugTestPending = true;
    ntpSyncRequested = true;
    debugActionRequested = static_cast<uint8_t>(DebugAction::Ntp);
    request->send(202, "application/json", "{\"requested\":true}");
  });

  server.on("/api/debug/ota-enable", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    otaEnableRequested = true;
    request->send(202, "application/json", "{\"requested\":true,\"seconds\":300}");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    if (LittleFS.exists("/index.html")) request->send(LittleFS, "/index.html", "text/html");
    else request->send(503, "text/plain", "Web UI is not installed. Run PlatformIO: Upload Filesystem Image.");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    request->send(LittleFS, "/script.js", "application/javascript");
  });
  server.begin();
}
