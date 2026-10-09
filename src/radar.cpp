#include "radar.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "geo.h"
#include "hal/hal.h"

namespace radar {
namespace {

constexpr char TAR1090_PATH[] = "/tar1090/data/aircraft.json";
constexpr uint16_t HTTP_TIMEOUT_MS = 4500;
constexpr int RADIUS = 100;

// Screen-shape dependent geometry. Landscape (W>H) keeps the original 320x240 layout;
// portrait moves the circle down so the header block has room above it.
struct Layout {
  int w, h, cx, cy;
};
Layout layout() {
  Layout l;
  l.w = hal::width();
  l.h = hal::height();
  l.cx = l.w / 2;
  l.cy = l.w > l.h ? 125 : 190;
  return l;
}

float radarRangeKm = 10.0f;
double radarLat = 0.0, radarLon = 0.0;
String receiverIp;
bool receiverOnline = false;
int totalPlanes = 0;
String overheadCallsign = "---";

void drawAircraft(float bearing, float distance, const char* label) {
  if (distance > radarRangeKm) return;
  auto& d = hal::display();
  const Layout L = layout();
  float a = (bearing - 90.0f) * DEG_TO_RAD, r = distance / radarRangeKm * RADIUS;
  int x = L.cx + (int)(cosf(a) * r), y = L.cy + (int)(sinf(a) * r);
  d.fillTriangle(x, y - 5, x - 4, y + 4, x + 4, y + 4, TFT_RED);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.setTextDatum(lgfx::top_left);
  d.drawString(label, constrain(x + 6, 0, L.w - 50), constrain(y - 8, 0, L.h - 15));
}

void setOffline() {
  receiverOnline = false;
  totalPlanes = 0;
  overheadCallsign = "---";
  drawShell();
}

}  // namespace

void begin(double lat, double lon, const String& receiverHost) {
  radarLat = lat;
  radarLon = lon;
  receiverIp = receiverHost;
}

void message(const char* a, const char* b) {
  auto& d = hal::display();
  const Layout L = layout();
  d.fillScreen(TFT_BLACK);
  d.setTextDatum(lgfx::middle_center);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(2);
  d.drawString(a, L.cx, L.h / 2 - 14);
  if (b) { d.setTextSize(1); d.drawString(b, L.cx, L.h / 2 + 16); }
}

void drawShell() {
  auto& d = hal::display();
  const Layout L = layout();
  const int CX = L.cx, CY = L.cy, W = L.w, H = L.h;
  d.fillScreen(TFT_BLACK);
  d.setTextDatum(lgfx::top_left);
  d.setTextSize(1);
  int line = 6;
  int battery = hal::batteryPercent();
  if (battery >= 0) {
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.drawString("BAT " + String(constrain(battery, 0, 100)) + "%", 6, line);
  }
  line += 14;
  d.setTextColor(receiverOnline ? TFT_GREEN : TFT_RED, TFT_BLACK);
  d.drawString(receiverOnline ? "Receiver online" : "Offline", 6, line);
  line += 14;
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.drawString("Total planes: " + String(totalPlanes), 6, line);
  line += 14;
  d.drawString("Over head: " + overheadCallsign, 6, line);
  d.drawCircle(CX, CY, RADIUS, TFT_GREEN);
  d.drawCircle(CX, CY, 75, TFT_DARKGREEN);
  d.drawCircle(CX, CY, 50, TFT_DARKGREEN);
  d.drawCircle(CX, CY, 25, TFT_DARKGREEN);
  d.drawLine(CX - RADIUS, CY, CX + RADIUS, CY, TFT_DARKGREEN);
  d.drawLine(CX, CY - RADIUS, CX, CY + RADIUS, TFT_DARKGREEN);
  d.setTextDatum(lgfx::top_center);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.drawString("N", CX, CY - RADIUS - 11);
  d.drawString("S", CX, CY + RADIUS + 2);
  d.setTextDatum(lgfx::middle_left);
  d.drawString("W", CX - 109, CY);
  d.drawString("E", CX + 104, CY);
  d.setTextDatum(lgfx::top_right);
  d.setTextColor(TFT_YELLOW, TFT_BLACK);
  d.drawString(String((int)radarRangeKm) + " km", W - 10, 8);
  d.setTextDatum(lgfx::bottom_center);
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString("Tap: range  Hold 5 s: menu", CX, H - 2);
}

void cycleRange() {
  radarRangeKm = radarRangeKm == 10 ? 25 : radarRangeKm == 25 ? 50 : radarRangeKm == 50 ? 100 : 10;
}

void fetch() {
  if (WiFi.status() != WL_CONNECTED || receiverIp.isEmpty()) {
    setOffline();
    return;
  }
  String url = "http://" + receiverIp + TAR1090_PATH;
  HTTPClient http;
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.begin(url);
  int status = http.GET();
  if (status == HTTP_CODE_OK) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (!error) {
      JsonArray aircraft = doc["aircraft"].as<JsonArray>();
      receiverOnline = true;
      totalPlanes = aircraft.size();
      overheadCallsign = "---";
      // Pass 1: status only (the shell shows the overhead callsign).
      double nearestOverheadKm = 2.0;
      for (JsonObject plane : aircraft) {
        if (!plane["lat"].is<double>() || !plane["lon"].is<double>()) continue;
        double d = haversineKm(radarLat, radarLon, plane["lat"], plane["lon"]);
        String callsign = plane["flight"] | "";
        callsign.trim();
        if (d <= nearestOverheadKm && !callsign.isEmpty()) {
          nearestOverheadKm = d;
          overheadCallsign = callsign;
        }
      }
      // Pass 2: shell first, then each plane exactly once on top.
      drawShell();
      int count = 0;
      for (JsonObject plane : aircraft) {
        if (!plane["lat"].is<double>() || !plane["lon"].is<double>()) continue;
        double d = haversineKm(radarLat, radarLon, plane["lat"], plane["lon"]);
        if (d > radarRangeKm) continue;
        const char* label = plane["flight"] | plane["hex"] | "AC";
        drawAircraft(bearingDeg(radarLat, radarLon, plane["lat"], plane["lon"]), d, label);
        count++;
      }
      Serial.printf("Received %d aircraft, plotted %d aircraft\n", totalPlanes, count);
    } else {
      setOffline();
      Serial.println("tar1090 returned invalid JSON");
    }
  } else {
    setOffline();
    Serial.printf("tar1090 request error: %d\n", status);
  }
  http.end();
}

}  // namespace radar
