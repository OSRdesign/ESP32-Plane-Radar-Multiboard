#pragma once
#include <Arduino.h>

namespace radar {

// Sets the radar centre and the tar1090 receiver host.
void begin(double lat, double lon, const String& receiverHost);

// Full-screen two-line status message (b optional).
void message(const char* a, const char* b = nullptr);

// Clears the screen and draws the status lines, rings and labels (no planes).
void drawShell();

// Cycles 10 -> 25 -> 50 -> 100 -> 10 km.
void cycleRange();

// Fetches aircraft.json (blocking, bounded by the HTTP timeout) and redraws
// the shell followed by the planes.
void fetch();

}  // namespace radar
