#ifdef BOARD_CYD
#include "hal.h"

// ESP32-2432S028 ("CYD"). Display on HSPI (SPI2_HOST), touch on its own VSPI (SPI3_HOST).
// Build flags: -DCYD_ST7789 swaps the panel driver; -DCYD_INVERT inverts colours (independent of the driver).

// Landscape rotation (0-3 normal, 4-7 mirrored in LovyanGFX). Verified on hardware:
// the ST7789 board is upright at 1, the ILI9341 board at 3. Override with -DCYD_ROTATION=n.
#ifndef CYD_ROTATION
#ifdef CYD_ST7789
#define CYD_ROTATION 1
#else
#define CYD_ROTATION 3
#endif
#endif

#ifndef CYD_TOUCH_OFFSET
#define CYD_TOUCH_OFFSET 6  // XPT2046 axes are swapped+flipped vs the panel on the CYD (common LovyanGFX CYD value)
#endif

namespace {
#ifdef CYD_ST7789
using CydPanel = lgfx::Panel_ST7789;
#else
using CydPanel = lgfx::Panel_ILI9341;
#endif
#ifdef CYD_INVERT
constexpr bool kInvert = true;
#else
constexpr bool kInvert = false;
#endif

class LGFX : public lgfx::LGFX_Device {
  CydPanel _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;
  lgfx::Touch_XPT2046 _touch;

 public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc = 2;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = 15;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = true;
      cfg.invert = kInvert;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl = 21;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      auto cfg = _touch.config();
      cfg.x_min = 300;
      cfg.x_max = 3900;
      cfg.y_min = 200;
      cfg.y_max = 3700;
      cfg.pin_int = 36;
      cfg.bus_shared = false;
      // Touch follows the display rotation (LovyanGFX convertRawXY). Override with -DCYD_TOUCH_OFFSET=n
      // if taps land mirrored/rotated on a board variant.
      cfg.offset_rotation = CYD_TOUCH_OFFSET;
      cfg.spi_host = SPI3_HOST;
      cfg.freq = 1000000;
      cfg.pin_sclk = 25;
      cfg.pin_mosi = 32;
      cfg.pin_miso = 39;
      cfg.pin_cs = 33;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
LGFX lcd;
}  // namespace

namespace hal {
void begin() {
  lcd.init();
  lcd.setRotation(CYD_ROTATION);  // board default; main applies the saved orientation
  lcd.setBrightness(255);
}
void update() {}
lgfx::LGFX_Device& display() { return lcd; }
int width() { return lcd.width(); }
int height() { return lcd.height(); }
int batteryPercent() { return -1; }
bool touchDown() {
  int32_t x, y;
  return lcd.getTouch(&x, &y) > 0;
}
bool touchPoint(int& x, int& y) {
  int32_t tx, ty;
  if (lcd.getTouch(&tx, &ty) == 0) return false;
  // LovyanGFX's offset_rotation 6 (swap + mirror) is right in the landscape rotations but leaves
  // the touch 180 deg off in the portrait ones (derived from Panel_Device::convertRawXY and
  // confirmed on hardware), so flip both axes when the display is taller than wide.
  if (lcd.width() < lcd.height()) {
    tx = lcd.width() - 1 - tx;
    ty = lcd.height() - 1 - ty;
  }
  x = tx;
  y = ty;
  return true;
}
void setOrientation(int n) {
  // Keep the mirror bit (4-7) of an overridden CYD_ROTATION; rotate the low two bits.
  lcd.setRotation((CYD_ROTATION & 4) | (((CYD_ROTATION & 3) + (n & 3)) & 3));
}
int defaultRotation() { return CYD_ROTATION; }
}  // namespace hal
#endif
