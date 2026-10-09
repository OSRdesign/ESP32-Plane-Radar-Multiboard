/*
 * Plane Radar
 * Live ADS-B aircraft from a tar1090 receiver on the same Wi-Fi network.
 * Inspired by MatixYo's ESP32-Plane-Radar:
 * https://github.com/MatixYo/ESP32-Plane-Radar
 *
 * First boot: join PlaneRadar-Setup and visit 192.168.4.1 to enter Wi-Fi
 * and radar-centre latitude/longitude. Tap the screen to cycle range.
 * Hold a finger on the screen for five seconds to clear setup data.
 */
#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hal/hal.h"
#include "radar.h"

namespace {
constexpr uint32_t FETCH_INTERVAL_MS = 5000;
constexpr uint32_t HOLD_CLEAR_MS = 5000;
constexpr uint32_t WIFI_CONNECT_MS = 15000;
constexpr uint32_t TOUCH_DEBOUNCE_MS = 50;

uint32_t lastFetchMs = 0;

// Debounced touch: the raw state must hold steady for TOUCH_DEBOUNCE_MS
// before it is accepted, so a noisy resistive panel does not fake taps.
bool touchRaw = false;
bool touchStable = false;
uint32_t touchRawSinceMs = 0;
uint32_t touchStartedMs = 0;

void onRelease(uint32_t held) {
  if (held >= HOLD_CLEAR_MS) {
    config::clear();
    radar::message("Setup cleared", "Restarting...");
    delay(500);
    ESP.restart();
  } else {
    radar::cycleRange();
    radar::drawShell();
    lastFetchMs = 0;
  }
}

void pollTouch() {
  uint32_t now = millis();
  bool raw = hal::touchDown();
  if (raw != touchRaw) {
    touchRaw = raw;
    touchRawSinceMs = now;
  }
  if (touchRaw == touchStable || now - touchRawSinceMs < TOUCH_DEBOUNCE_MS) return;
  touchStable = touchRaw;
  if (touchStable) {
    touchStartedMs = touchRawSinceMs;
  } else {
    onRelease(touchRawSinceMs - touchStartedMs);
  }
}
}  // namespace

void setup() {
  hal::begin();
  Serial.begin(115200);
  radar::drawShell();
  config::Settings s;
  bool configured = config::load(s);
  radar::begin(s.lat, s.lon, s.receiver);
  if (!configured) {
    config::startPortal();
    radar::message("PlaneRadar-Setup", "Join Wi-Fi, then open 192.168.4.1");
    return;
  }
  radar::message("Connecting to Wi-Fi...", s.ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(s.ssid.c_str(), s.pass.c_str());
  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_MS) delay(100);
  if (WiFi.status() != WL_CONNECTED) {
    config::startPortal();
    radar::message("PlaneRadar-Setup", "Join Wi-Fi, then open 192.168.4.1");
  } else {
    radar::drawShell();
  }
}

void loop() {
  hal::update();
  if (config::portalActive()) { config::handlePortal(); return; }
  pollTouch();
  // Fetch blocks for up to the HTTP timeout; never start one while a finger
  // is on the screen so a tap's release is handled without that delay.
  if (!touchRaw && !touchStable && millis() - lastFetchMs >= FETCH_INTERVAL_MS) {
    lastFetchMs = millis();
    radar::fetch();
  }
}
