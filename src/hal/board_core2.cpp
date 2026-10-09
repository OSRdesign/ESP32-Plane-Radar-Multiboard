#ifdef BOARD_CORE2
#include "hal.h"

namespace hal {
void begin() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);  // landscape 320x240
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
}  // namespace hal
#endif
