#pragma once
#include <Arduino.h>

namespace config {

struct Settings {
  String ssid;
  String pass;
  String receiver;  // host[:port], no scheme, no path
  double lat = 0.0;
  double lon = 0.0;
  uint8_t rotation = 0;  // user orientation 0..3 (see hal::setOrientation)
};

// Reads settings from NVS (namespace "planeradar").
// Returns true when ssid and receiver are both present.
bool load(Settings& out);

// Persists the user orientation (0..3) without touching other keys.
void saveRotation(int n);

// Erases all stored settings (including orientation).
void clear();

// Starts the "PlaneRadar-Setup" access point and the form on 192.168.4.1.
void startPortal();

// True after startPortal(); service with handlePortal() from loop().
bool portalActive();
void handlePortal();

}  // namespace config
