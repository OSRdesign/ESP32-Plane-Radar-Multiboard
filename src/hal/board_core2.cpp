#ifdef BOARD_CORE2
#include "hal.h"

namespace hal {
namespace {
constexpr int kDefaultRotation = 1;
}
void begin() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(kDefaultRotation);  // landscape 320x240
}
void update() { M5.update(); }
lgfx::LGFX_Device& display() { return M5.Display; }
int width() { return M5.Display.width(); }
int height() { return M5.Display.height(); }
int batteryPercent() {
  int32_t p = M5.Power.getBatteryLevel();
  if (p < 0) return 0;
  return p > 100 ? 100 : (int)p;
}
bool touchDown() { return M5.Touch.getCount() > 0; }
bool touchPoint(int& x, int& y) {
  if (M5.Touch.getCount() == 0) return false;
  auto t = M5.Touch.getDetail(0);  // coordinates already follow the display rotation
  x = t.x;
  y = t.y;
  return true;
}
void setOrientation(int n) { M5.Display.setRotation((kDefaultRotation + (n & 3)) & 3); }
int defaultRotation() { return kDefaultRotation; }
}  // namespace hal
#endif
