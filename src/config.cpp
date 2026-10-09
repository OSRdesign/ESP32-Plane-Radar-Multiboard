#include "config.h"

#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <math.h>
#include <stdlib.h>

namespace config {
namespace {

constexpr char AP_SSID[] = "PlaneRadar-Setup";
constexpr char NVS_NAMESPACE[] = "planeradar";

Preferences prefs;
WebServer configServer(80);
bool active = false;

const char CONFIG_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Plane Radar setup</title><style>body{font-family:sans-serif;max-width:380px;margin:32px auto;padding:0 16px}input,button{box-sizing:border-box;width:100%;padding:10px;margin:5px 0 14px;font-size:16px}button{border:0;background:#078a42;color:white}</style></head><body><h2>Plane Radar setup</h2><p>Enter the same Wi-Fi used by the ADS-B receiver, its local IP address, and the latitude and longitude at the centre of this radar.</p><form method="post" action="/save"><label>Wi-Fi name</label><input name="ssid" required><label>Wi-Fi password</label><input name="pass" type="password"><label>Receiver IP address</label><input name="receiver" placeholder="192.168.1.50" inputmode="decimal" required><label>Latitude</label><input name="lat" placeholder="52.3676" required><label>Longitude</label><input name="lon" placeholder="4.9041" required><button>Save and connect</button></form></body></html>
)HTML";

// Parses a whole-string decimal number within [lo, hi].
bool parseCoord(String s, double lo, double hi, double& out) {
  s.trim();
  if (s.isEmpty()) return false;
  const char* begin = s.c_str();
  char* end = nullptr;
  double v = strtod(begin, &end);
  if (end == begin || *end != '\0' || isnan(v)) return false;
  if (v < lo || v > hi) return false;
  out = v;
  return true;
}

void sendError(const char* msg) {
  String body = "<h2>Invalid setup</h2><p>";
  body += msg;
  body += "</p><p><a href=\"/\">Back</a></p>";
  configServer.send(400, "text/html", body);
}

void handleRoot() { configServer.send(200, "text/html", CONFIG_HTML); }

void handleSave() {
  String ssid = configServer.arg("ssid");
  String receiver = configServer.arg("receiver");
  receiver.trim();
  if (receiver.startsWith("http://")) receiver.remove(0, 7);
  if (receiver.startsWith("https://")) receiver.remove(0, 8);
  int slash = receiver.indexOf('/');
  if (slash >= 0) receiver.remove(slash);

  double lat = 0.0, lon = 0.0;
  if (ssid.isEmpty()) { sendError("Wi-Fi name is required."); return; }
  if (receiver.isEmpty()) { sendError("Receiver IP address is required."); return; }
  if (!parseCoord(configServer.arg("lat"), -90.0, 90.0, lat)) {
    sendError("Latitude must be a number between -90 and 90."); return;
  }
  if (!parseCoord(configServer.arg("lon"), -180.0, 180.0, lon)) {
    sendError("Longitude must be a number between -180 and 180."); return;
  }

  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", configServer.arg("pass"));
  prefs.putString("receiver", receiver);
  prefs.putDouble("lat", lat);
  prefs.putDouble("lon", lon);
  prefs.end();
  configServer.send(200, "text/html", "<h2>Saved. The radar is restarting.</h2>");
  delay(1000);
  ESP.restart();
}

}  // namespace

bool load(Settings& out) {
  prefs.begin(NVS_NAMESPACE, true);
  out.ssid = prefs.getString("ssid", "");
  out.pass = prefs.getString("pass", "");
  out.lat = prefs.getDouble("lat", 0);
  out.lon = prefs.getDouble("lon", 0);
  out.receiver = prefs.getString("receiver", "");
  out.rotation = prefs.getUChar("rot", 0) & 3;
  prefs.end();
  return !out.ssid.isEmpty() && !out.receiver.isEmpty();
}

void saveRotation(int n) {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putUChar("rot", (uint8_t)(n & 3));
  prefs.end();
}

void clear() {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}

void startPortal() {
  active = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  configServer.on("/", HTTP_GET, handleRoot);
  configServer.on("/save", HTTP_POST, handleSave);
  configServer.begin();
}

bool portalActive() { return active; }

void handlePortal() { configServer.handleClient(); }

}  // namespace config
