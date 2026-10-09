#ifdef BOARD_CYD
#include "hal.h"

// ESP32-2432S028 ("CYD"). Display on HSPI (SPI2_HOST), touch on its own VSPI (SPI3_HOST).
// Build flags: -DCYD_ST7789 swaps the panel driver; -DCYD_INVERT inverts colours (independent of the driver).

#ifndef CYD_ROTATION
#define CYD_ROTATION 1  // 0-3 normal, 4-7 mirrored (LovyanGFX)
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
      cfg.offset_rotation = 0;  // UNVERIFIED on hardware; only touchDown() is used by the app
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
  lcd.setRotation(CYD_ROTATION);
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
}  // namespace hal
#endif
