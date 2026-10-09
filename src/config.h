#pragma once
#include <Arduino.h>

namespace config {

struct Settings {
  String ssid;
  String pass;
  String receiver;  // host[:port], no scheme, no path
  double lat = 0.0;
  double lon = 0.0;
};

// Reads settings from NVS (namespace "planeradar").
// Returns true when ssid and receiver are both present.
bool load(Settings& out);

// Erases all stored settings.
void clear();

// Starts the "PlaneRadar-Setup" access point and the form on 192.168.4.1.
void startPortal();

// True after startPortal(); service with handlePortal() from loop().
bool portalActive();
void handlePortal();

}  // namespace config
