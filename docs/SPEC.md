# Plane Radar ESP32 — Spec

## Goals
- Port the existing M5Stack Core2 sketch (`legacy/core2_original.cpp`) to a multi-board PlatformIO project.
- First new target: Cheap Yellow Display (CYD) ESP32-2432S028 / 2435S028 (2.8" 320x240, ESP32-WROOM, ILI9341 over SPI, XPT2046 resistive touch, no battery gauge).
- Behaviour parity on all boards: Wi-Fi setup portal, poll tar1090 every 5 s, radar plot with 4 ranges (10/25/50/100 km), "overhead" callsign (<2 km), tap = cycle range, 5 s hold = clear config.
- Documented and published as an MIT-licensed public GitHub repo.

## Non-goals
- No new features (no altitude/speed UI, no OpenSky, no HTTPS) in this port; list them as README roadmap only.
- No Arduino IDE support.

## Stack
PlatformIO, `espressif32` platform, framework arduino. Libraries: ArduinoJson ^7, LovyanGFX ^1.2 (CYD), M5Unified + M5GFX (Core2). Libraries pinned in platformio.ini. Env `core2`: board `m5stack-core2`. Env `cyd`: board `esp32dev`.

## Architecture
- `hal.h` — namespace `hal`: `void begin()`, `lgfx::LGFX_Device& display()`, `int width()/height()`, `int batteryPercent()` (-1 none), `bool touchDown()` (poll, debounced by caller), `void update()` (per-loop; M5.update on Core2).
- Both M5GFX and LovyanGFX derive from `lgfx::LGFX_Device`, so all drawing code is shared. Core2 uses M5Unified for power/touch init; CYD defines an `LGFX` class (ILI9341 SPI pins SCLK14 MOSI13 MISO12 CS15 DC2, backlight GPIO21, XPT2046 CS33 IRQ36 on SPI bus 12/13/14 — verify; some CYD variants need `invert=true` or ST7789 driver, selectable via build flag `CYD_INVERT`/`CYD_ST7789`, documented in BOARDS.md).
- `geo` — haversine/bearing (moved verbatim).
- `config` — NVS (namespace `planeradar`, same keys as original: ssid, pass, receiver, lat, lon), setup AP `PlaneRadar-Setup`, 192.168.4.1 form. Fix: HTML-escape nothing needed but validate lat/lon ranges and non-empty fields; keep behaviour.
- `radar` — fetch + render. Fix vs original: draw each plane once (original draws twice, the first pass is erased by `drawRadarShell`); render into an off-screen sprite or at least avoid full-screen flicker where RAM allows (320x240x16bit = 150 KB; CYD without PSRAM may not fit — use fillScreen-less partial redraw of the radar area, or fall back to direct draw). Keep it simple: direct draw, no sprite, acceptable.
- Layout is identical to the original (CX=160, CY=125, R=100). Battery line is omitted when `batteryPercent()==-1`.
- Touch: CYD uses XPT2046 via LovyanGFX `getTouch`; Core2 via `M5.Touch.getCount()`. Gesture logic (tap vs 5 s hold) in main.cpp using `hal::touchDown()`.

## Definition of done
- `pio run -e core2` and `pio run -e cyd` both build with no warnings.
- Behaviour matches original (verified by code review; hardware test by the user — flash and observe).
- README, BOARDS.md, LICENSE present; public GitHub repo pushed.
