/*
 * M5Stack Core2 Plane Radar
 * Live ADS-B aircraft from a tar1090 receiver on the same Wi-Fi network.
 * Inspired by MatixYo's ESP32-Plane-Radar:
 * https://github.com/MatixYo/ESP32-Plane-Radar
 *
 * First boot: join PlaneRadar-Setup and visit 192.168.4.1 to enter Wi-Fi
 * and radar-centre latitude/longitude. Tap the screen to cycle range.
 * Hold a finger on the screen for five seconds to clear setup data.
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>


// Forward declarations
void message(const char *a, const char *b);
double haversineKm(double lat1, double lon1, double lat2, double lon2);
double bearingDeg(double lat1, double lon1, double lat2, double lon2);
void drawRadarShell();
void drawAircraft(float bearing, float distance, const char *label);
void handleRoot();
void handleSave();
void startPortal();
void fetchAircraft();

constexpr uint32_t FETCH_INTERVAL_MS = 5000;
constexpr uint32_t HOLD_CLEAR_MS = 5000;
constexpr char AP_SSID[] = "PlaneRadar-Setup";
constexpr char NVS_NAMESPACE[] = "planeradar";
constexpr char TAR1090_PATH[] = "/tar1090/data/aircraft.json";
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
constexpr int CX = 160;
constexpr int CY = 125;
constexpr int RADIUS = 100;
constexpr double DEG2RAD = M_PI / 180.0;

M5Canvas canvas(&M5.Display);
Preferences prefs;
WebServer configServer(80);
float radarRangeKm = 10.0f;
double radarLat = 0.0, radarLon = 0.0;
String receiverIp;
bool configMode = false;
bool receiverOnline = false;
int totalPlanes = 0;
String overheadCallsign = "---";
uint32_t lastFetchMs = 0, touchStartedMs = 0;
bool touchWasDown = false;

const char CONFIG_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Plane Radar setup</title><style>body{font-family:sans-serif;max-width:380px;margin:32px auto;padding:0 16px}input,button{box-sizing:border-box;width:100%;padding:10px;margin:5px 0 14px;font-size:16px}button{border:0;background:#078a42;color:white}</style></head><body><h2>Plane Radar setup</h2><p>Enter the same Wi-Fi used by the ADS-B receiver, its local IP address, and the latitude and longitude at the centre of this radar.</p><form method="post" action="/save"><label>Wi-Fi name</label><input name="ssid" required><label>Wi-Fi password</label><input name="pass" type="password"><label>Receiver IP address</label><input name="receiver" placeholder="192.168.1.50" inputmode="decimal" required><label>Latitude</label><input name="lat" placeholder="52.3676" required><label>Longitude</label><input name="lon" placeholder="4.9041" required><button>Save and connect</button></form></body></html>
)HTML";

void message(const char *a, const char *b = nullptr) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.drawString(a, CX, 106);
  if (b) { M5.Display.setTextSize(1); M5.Display.drawString(b, CX, 136); }
}

double haversineKm(double lat1, double lon1, double lat2, double lon2) {
  double dlat = (lat2-lat1)*DEG2RAD, dlon = (lon2-lon1)*DEG2RAD;
  double a = sin(dlat/2)*sin(dlat/2) + cos(lat1*DEG2RAD)*cos(lat2*DEG2RAD)*sin(dlon/2)*sin(dlon/2);
  return 6371.0 * 2.0 * atan2(sqrt(a), sqrt(1.0-a));
}
double bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double dlon=(lon2-lon1)*DEG2RAD;
  double y=sin(dlon)*cos(lat2*DEG2RAD);
  double x=cos(lat1*DEG2RAD)*sin(lat2*DEG2RAD)-sin(lat1*DEG2RAD)*cos(lat2*DEG2RAD)*cos(dlon);
  return fmod(atan2(y,x)/DEG2RAD+360.0,360.0);
}

void drawRadarShell() {
  M5.Display.fillScreen(TFT_BLACK);
  int batteryPercent = constrain(M5.Power.getBatteryLevel(), 0, 100);
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString("BAT " + String(batteryPercent) + "%", 6, 6);
  M5.Display.setTextColor(receiverOnline ? TFT_GREEN : TFT_RED, TFT_BLACK);
  M5.Display.drawString(receiverOnline ? "Receiver online" : "Offline", 6, 20);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawString("Total planes: " + String(totalPlanes), 6, 34);
  M5.Display.drawString("Over head: " + overheadCallsign, 6, 48);
  M5.Display.drawCircle(CX, CY, RADIUS, TFT_GREEN);
  M5.Display.drawCircle(CX, CY, 75, TFT_DARKGREEN);
  M5.Display.drawCircle(CX, CY, 50, TFT_DARKGREEN);
  M5.Display.drawCircle(CX, CY, 25, TFT_DARKGREEN);
  M5.Display.drawLine(CX-RADIUS, CY, CX+RADIUS, CY, TFT_DARKGREEN);
  M5.Display.drawLine(CX, CY-RADIUS, CX, CY+RADIUS, TFT_DARKGREEN);
  M5.Display.setTextDatum(top_center); M5.Display.setTextColor(TFT_WHITE,TFT_BLACK); M5.Display.setTextSize(1);
  M5.Display.drawString("N",CX,14); M5.Display.drawString("S",CX,227);
  M5.Display.setTextDatum(middle_left); M5.Display.drawString("W",51,CY); M5.Display.drawString("E",264,CY);
  M5.Display.setTextDatum(top_right); M5.Display.setTextColor(TFT_YELLOW,TFT_BLACK);
  M5.Display.drawString(String((int)radarRangeKm)+" km",310,8);
  M5.Display.setTextDatum(bottom_center); M5.Display.setTextColor(TFT_DARKGREY,TFT_BLACK);
  M5.Display.drawString("Tap: range  Hold 5 s: setup",CX,238);
}
void drawAircraft(float bearing, float distance, const char *label) {
  if (distance > radarRangeKm) return;
  float a=(bearing-90.0f)*DEG_TO_RAD, r=distance/radarRangeKm*RADIUS;
  int x=CX+(int)(cosf(a)*r), y=CY+(int)(sinf(a)*r);
  M5.Display.fillTriangle(x,y-5,x-4,y+4,x+4,y+4,TFT_RED);
  M5.Display.setTextColor(TFT_WHITE,TFT_BLACK); M5.Display.setTextSize(1); M5.Display.setTextDatum(top_left);
  M5.Display.drawString(label, constrain(x+6,0,270), constrain(y-8,0,225));
}
void handleRoot(){ configServer.send(200,"text/html",CONFIG_HTML); }
void handleSave() {
  prefs.begin(NVS_NAMESPACE,false);
  String receiver = configServer.arg("receiver");
  receiver.trim();
  if (receiver.startsWith("http://")) receiver.remove(0, 7);
  if (receiver.startsWith("https://")) receiver.remove(0, 8);
  int slash = receiver.indexOf('/');
  if (slash >= 0) receiver.remove(slash);
  prefs.putString("ssid",configServer.arg("ssid")); prefs.putString("pass",configServer.arg("pass"));
  prefs.putString("receiver",receiver);
  prefs.putDouble("lat",configServer.arg("lat").toDouble()); prefs.putDouble("lon",configServer.arg("lon").toDouble());
  prefs.end(); configServer.send(200,"text/html","<h2>Saved. The radar is restarting.</h2>"); delay(1000); ESP.restart();
}
void startPortal() {
  configMode=true; WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID);
  configServer.on("/",HTTP_GET,handleRoot); configServer.on("/save",HTTP_POST,handleSave); configServer.begin();
  message("PlaneRadar-Setup","Join Wi-Fi, then open 192.168.4.1");
}
void fetchAircraft() {
  if(WiFi.status()!=WL_CONNECTED || receiverIp.isEmpty()) {
    receiverOnline = false;
    totalPlanes = 0;
    overheadCallsign = "---";
    drawRadarShell();
    return;
  }
  String url = "http://" + receiverIp + TAR1090_PATH;
  HTTPClient http; http.begin(url); http.setTimeout(4500); int status=http.GET();
  if(status==HTTP_CODE_OK) {
    JsonDocument doc; DeserializationError error=deserializeJson(doc,http.getStream());
    if(!error) {
      JsonArray aircraft = doc["aircraft"].as<JsonArray>();
      receiverOnline = true;
      totalPlanes = aircraft.size();
      overheadCallsign = "---";
      double nearestOverheadKm = 2.0;
      int count=0;
      for(JsonObject plane: aircraft) {
        if(!plane["lat"].is<double>()||!plane["lon"].is<double>()) continue;
        double d=haversineKm(radarLat,radarLon,plane["lat"],plane["lon"]);
        String callsign = plane["flight"] | "";
        callsign.trim();
        if(d <= nearestOverheadKm && !callsign.isEmpty()) {
          nearestOverheadKm = d;
          overheadCallsign = callsign;
        }
        if(d>radarRangeKm) continue;
        const char* label=plane["flight"]|plane["hex"]|"AC";
        drawAircraft(bearingDeg(radarLat,radarLon,plane["lat"],plane["lon"]),d,label); count++;
      }
      drawRadarShell();
      for(JsonObject plane: aircraft) {
        if(!plane["lat"].is<double>()||!plane["lon"].is<double>()) continue;
        double d=haversineKm(radarLat,radarLon,plane["lat"],plane["lon"]); if(d>radarRangeKm) continue;
        const char* label=plane["flight"]|plane["hex"]|"AC";
        drawAircraft(bearingDeg(radarLat,radarLon,plane["lat"],plane["lon"]),d,label);
      }
      Serial.printf("Received %d aircraft, plotted %d aircraft\n",totalPlanes,count);
    } else {
      receiverOnline = false;
      totalPlanes = 0;
      overheadCallsign = "---";
      drawRadarShell();
      Serial.println("tar1090 returned invalid JSON");
    }
  } else {
    receiverOnline = false;
    totalPlanes = 0;
    overheadCallsign = "---";
    drawRadarShell();
    Serial.printf("tar1090 request error: %d\n",status);
  }
  http.end();
}
void setup() {
  auto cfg=M5.config(); M5.begin(cfg); M5.Display.setRotation(1); Serial.begin(115200);
  drawRadarShell(); prefs.begin(NVS_NAMESPACE,true); String ssid=prefs.getString("ssid",""); String pass=prefs.getString("pass","");
  radarLat=prefs.getDouble("lat",0); radarLon=prefs.getDouble("lon",0); receiverIp=prefs.getString("receiver",""); prefs.end();
  if(ssid.isEmpty() || receiverIp.isEmpty()) { startPortal(); return; }
  message("Connecting to Wi-Fi...",ssid.c_str()); WiFi.mode(WIFI_STA); WiFi.begin(ssid.c_str(),pass.c_str());
  uint32_t started=millis(); while(WiFi.status()!=WL_CONNECTED && millis()-started<15000) delay(100);
  if(WiFi.status()!=WL_CONNECTED) startPortal(); else drawRadarShell();
}
void loop() {
  M5.update();
  if(configMode) { configServer.handleClient(); return; }
  bool down=M5.Touch.getCount()>0;
  if(down&&!touchWasDown) { touchStartedMs=millis(); touchWasDown=true; }
  if(!down&&touchWasDown) {
    uint32_t held=millis()-touchStartedMs; touchWasDown=false;
    if(held>=HOLD_CLEAR_MS) { prefs.begin(NVS_NAMESPACE,false); prefs.clear(); prefs.end(); message("Setup cleared","Restarting..."); delay(500); ESP.restart(); }
    else { radarRangeKm = radarRangeKm==10?25:radarRangeKm==25?50:radarRangeKm==50?100:10; drawRadarShell(); lastFetchMs=0; }
  }
  if(millis()-lastFetchMs>=FETCH_INTERVAL_MS) { lastFetchMs=millis(); fetchAircraft(); }
}