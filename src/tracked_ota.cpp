#include "tracked_ota.h"

#ifndef BOT_OTA_HOSTNAME
#define BOT_OTA_HOSTNAME "tracked-bot"
#endif

#if defined(ENABLE_OTA) && defined(MICRO_ROS_WIFI)

#include <ArduinoOTA.h>
#include <WiFi.h>

namespace {

bool s_ota_ok = false;
bool s_ota_busy = false;
bool s_ota_started = false;

void start_ota_server() {
  if (s_ota_started) {
    return;
  }
  s_ota_started = true;

  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setHostname(BOT_OTA_HOSTNAME);
  ArduinoOTA.setHostname(BOT_OTA_HOSTNAME);
#if defined(BOT_OTA_PASS)
  ArduinoOTA.setPassword(BOT_OTA_PASS);
#endif
  ArduinoOTA.onStart([]() {
    s_ota_busy = true;
    WiFi.setSleep(WIFI_PS_NONE);
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    s_ota_busy = false;
    Serial.println("OTA end — rebooting");
  });
  ArduinoOTA.onError([](ota_error_t err) {
    s_ota_busy = false;
    Serial.printf("OTA err %u\n", err);
  });
  ArduinoOTA.begin();
  s_ota_ok = true;
  Serial.printf("OTA ready — %s.local  IP %s\n", BOT_OTA_HOSTNAME,
                WiFi.localIP().toString().c_str());
}

}  // namespace

void tracked_ota_begin() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA: WiFi not ready — will retry in loop");
    return;
  }
  start_ota_server();
}

void tracked_ota_tick() {
  if (!s_ota_ok && WiFi.status() == WL_CONNECTED) {
    start_ota_server();
  }
  if (s_ota_ok) {
    ArduinoOTA.handle();
  }
}

bool tracked_ota_busy() {
  return s_ota_busy;
}

#else

void tracked_ota_begin() {}
void tracked_ota_tick() {}
bool tracked_ota_busy() {
  return false;
}

#endif
