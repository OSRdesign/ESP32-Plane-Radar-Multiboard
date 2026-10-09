# Supported boards

Every board runs the same app code. Board-specific code lives only in `src/hal/board_*.cpp`, chosen
by a build flag set in the board's PlatformIO env. All boards default to a 320x240 landscape screen;
the user can rotate it (also to portrait, 240x320) from the on-screen menu.

## M5Stack Core2

| Item | Value |
|---|---|
| PlatformIO env | `core2` (board `m5stack-core2`, flag `-DBOARD_CORE2`) |
| MCU | ESP32-D0WDQ6-V3, 16 MB flash, 8 MB PSRAM |
| Display | 2.0" 320x240 IPS (ILI9342C) |
| Touch | FT6336U capacitive |
| Battery | Yes, via the power chip (AXP192 on v1.0, AXP2101 on v1.1; M5Unified handles both); shown as `BAT nn%` |
| Libraries | M5Unified + M5GFX |
| Implementation | `src/hal/board_core2.cpp` |

All pins, power management and touch are handled by M5Unified (`M5.begin()`); the project does not
define any Core2 pins itself.

## Cheap Yellow Display (ESP32-2432S028 / "2435S028")

| Item | Value |
|---|---|
| PlatformIO env | `cyd` (board `esp32dev`, flag `-DBOARD_CYD`); variants `cyd_invert`, `cyd_st7789` |
| MCU | ESP32-WROOM-32 (no PSRAM) |
| Display | 2.8" 320x240 ILI9341 over SPI |
| Touch | XPT2046 resistive, on its own SPI bus |
| Battery | None; the battery line is hidden |
| Libraries | LovyanGFX |
| Implementation | `src/hal/board_cyd.cpp` |

### Display (HSPI / `SPI2_HOST`)

| Signal | GPIO |
|---|---|
| SCLK | 14 |
| MOSI | 13 |
| MISO | 12 |
| CS | 15 |
| DC | 2 |
| RST | not connected (-1) |
| Backlight | 21 (PWM) |

### Touch, XPT2046 (VSPI / `SPI3_HOST`, separate from the display bus)

| Signal | GPIO |
|---|---|
| CLK | 25 |
| MOSI | 32 |
| MISO | 39 |
| CS | 33 |
| IRQ | 36 |

The menu needs touch coordinates (`hal::touchPoint`). LovyanGFX rotates them together with the
display rotation, using the `x_min/x_max/y_min/y_max` calibration in `board_cyd.cpp`. If taps land
mirrored or rotated on a variant, set `-DCYD_TOUCH_OFFSET=n` (touch `offset_rotation`, 0-7) in the env.
Touch and the menu are verified in all four orientations on all supported boards: Core2, and both CYD variants (ILI9341 `cyd`, ST7789 `cyd_st7789`).

### Known CYD variants

The "CYD" name covers several slightly different boards sold under the same part number.

| Variant | Symptom with `cyd` | What to use |
|---|---|---|
| ILI9341 panel with inverted colours | Colours inverted (black shows white) | env `cyd_invert` (flag `-DCYD_INVERT`) |
| ST7789 panel clone | Mirrored/rotated, blank or garbled image with the default env | env `cyd_st7789` (flag `-DCYD_ST7789`; combine with `-DCYD_INVERT` if colours are inverted). Verified on a 2-USB-port CYD |
| Backlight on GPIO27 | Screen stays dark even though the board runs (serial log OK) | No env yet: change `cfg.pin_bl = 21;` to `27` in `src/hal/board_cyd.cpp` and rebuild |

## Adding a new board

1. **Create `src/hal/board_x.cpp`** wrapped in `#ifdef BOARD_X ... #endif`, implementing every
   function declared in `src/hal/hal.h`:
   - `void begin()`: initialise display (default landscape, 320x240), touch, backlight.
   - `void update()`: per-loop housekeeping (may be empty).
   - `lgfx::LGFX_Device& display()`: the display object. LovyanGFX and M5GFX devices both derive
     from `lgfx::LGFX_Device`, so the shared drawing code works unchanged.
   - `int width()` / `int height()`: screen size in the current rotation.
   - `int batteryPercent()`: 0-100, or `-1` if the board has no battery gauge (the UI then hides it).
   - `bool touchDown()`: `true` while the screen is touched (no debouncing needed; the caller handles
     gestures).
   - `bool touchPoint(int& x, int& y)`: like `touchDown()` plus coordinates in the current, rotated
     display space.
   - `void setOrientation(int n)` / `int defaultRotation()`: absolute rotation is
     `(defaultRotation() + n) % 4` for user orientation `n` 0..3.
2. **Include the board's graphics library in `src/hal/hal.h`**: add an
   `#elif defined(BOARD_X)` branch next to the existing `BOARD_CORE2` / `BOARD_CYD` ones.
3. **Add an env to `platformio.ini`**:

   ```ini
   [env:x]
   board = <platformio-board-id>
   build_flags = -DBOARD_X
   lib_deps =
       lovyan03/LovyanGFX@^1.2.0
       bblanchon/ArduinoJson@^7
   ```

4. **Build** with `pio run -e x` and make sure it compiles without warnings; also rebuild the existing
   envs to check nothing else broke.
5. **Document it**: add a row to the "Supported boards" table in `README.md`, a section in this file
   with its pins and quirks, and the new file in `filemap.md`.

Keep all board specifics inside `board_x.cpp`; the app code in `src/` must stay board-agnostic and
should not hardcode the screen size.
