# Plane Radar ESP32 — Spec

## Goals
- Port the existing M5Stack Core2 sketch (`legacy/core2_original.cpp`) to a multi-board PlatformIO project.
- First new target: Cheap Yellow Display (CYD) ESP32-2432S028 / 2435S028 (2.8" 320x240, ESP32-WROOM, ILI9341 over SPI, XPT2046 resistive touch, no battery gauge).
- Behaviour parity on all boards: Wi-Fi setup portal, poll tar1090 every 5 s, radar plot with 4 ranges (10/25/50/100 km), "overhead" callsign (<2 km), tap = cycle range, 5 s hold = on-screen menu.
- Documented and published as an MIT-licensed public GitHub repo.

## Non-goals
- No new features (no altitude/speed UI, no OpenSky, no HTTPS) in this port; list them as README roadmap only.
- No Arduino IDE support.

## Stack
PlatformIO, `espressif32` platform, framework arduino. Libraries: ArduinoJson ^7, LovyanGFX ^1.2 (CYD), M5Unified + M5GFX (Core2). Libraries pinned in platformio.ini. Env `core2`: board `m5stack-core2`. Env `cyd`: board `esp32dev`.

## Architecture
- `hal.h` — namespace `hal`: `void begin()`, `lgfx::LGFX_Device& display()`, `int width()/height()`, `int batteryPercent()` (-1 none), `bool touchDown()` (poll, debounced by caller), `bool touchPoint(int& x,int& y)` (pressed + coordinates in the current rotated display space), `void setOrientation(int n)` (n 0..3, absolute rotation = (board default + n) % 4; default 1 for Core2 and CYD ST7789, 3 for CYD ILI9341), `int defaultRotation()`, `void update()` (per-loop; M5.update on Core2).
- Both M5GFX and LovyanGFX derive from `lgfx::LGFX_Device`, so all drawing code is shared. Core2 uses M5Unified for power/touch init; CYD defines an `LGFX` class (ILI9341 SPI pins SCLK14 MOSI13 MISO12 CS15 DC2, backlight GPIO21, XPT2046 CS33 IRQ36 on SPI bus 12/13/14 — verify; some CYD variants need `invert=true` or ST7789 driver, selectable via build flag `CYD_INVERT`/`CYD_ST7789`, documented in BOARDS.md).
- `geo` — haversine/bearing (moved verbatim).
- `config` — NVS (namespace `planeradar`, same keys as original: ssid, pass, receiver, lat, lon), setup AP `PlaneRadar-Setup`, 192.168.4.1 form. Fix: HTML-escape nothing needed but validate lat/lon ranges and non-empty fields; keep behaviour.
- `radar` — fetch + render. Fix vs original: draw each plane once (original draws twice, the first pass is erased by `drawRadarShell`); render into an off-screen sprite or at least avoid full-screen flicker where RAM allows (320x240x16bit = 150 KB; CYD without PSRAM may not fit — use fillScreen-less partial redraw of the radar area, or fall back to direct draw). Keep it simple: direct draw, no sprite, acceptable.
- Landscape layout follows the original (CX=160, CY=120, R=100; the circle sits 5 px higher so the S label is not hidden by the hint). Portrait (240x320): R=100, CX=W/2, CY=190; header, range label, N/S/W/E and hint positions derive from the same `Layout` struct (width/height), never hardcoded 320/240. Battery line is omitted when `batteryPercent()==-1`.
- Touch: CYD uses XPT2046 via LovyanGFX `getTouch`; Core2 via `M5.Touch.getCount()`. Gesture logic (tap vs 5 s hold) in main.cpp using `hal::touchDown()`; coordinates (`touchPoint`) are only used by the menu.

## Menu and orientation
- Hold >= 5 s, then release: `menu::run()` (src/menu.cpp), a blocking modal loop with its own 50 ms debounce. The release that opened it is swallowed and the finger must lift before the first tap counts. A button fires when press and release are inside the same button.
- 2x2 orientation buttons (Landscape, Portrait, Landscape flip, Portrait flip = n 0..3); the selected one is green. Tapping applies immediately (`hal::setOrientation`) and saves to NVS key `rot` (uint8, default 0, namespace `planeradar`); the menu redraws in the new orientation. `config::Settings::rotation` is loaded at boot and applied by main; the portal save leaves `rot` untouched; `config::clear()` wipes it.
- "Re-run setup" needs two taps (first turns it into red "Tap again"; any other tap or 4 s cancels); then `config::clear()` and restart. "Back" redraws the radar and forces an immediate fetch.
- Touch: LovyanGFX converts touch with the display rotation; M5Unified converts via `convertRawXY` as well.

## Definition of done
- `pio run -e core2` and `pio run -e cyd` both build with no warnings.
- Behaviour matches original (verified by code review; hardware test by the user — flash and observe).
- README, BOARDS.md, LICENSE present; public GitHub repo pushed.
