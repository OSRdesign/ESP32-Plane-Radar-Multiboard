# Plane Radar ESP32

ESP32 touchscreen ADS-B radar: polls a local tar1090 `aircraft.json`, plots planes around a configured lat/lon. Multi-board via PlatformIO envs (M5Stack Core2, Cheap Yellow Display ESP32-2432S028 / "2435S028").

- Stack: Arduino + PlatformIO, ArduinoJson 7, LovyanGFX (CYD) / M5Unified+M5GFX (Core2).
- Build: `pio run -e cyd` / `pio run -e core2`. Flash: `pio run -e <env> -t upload`. Lint-ish check: both envs must compile warning-free.
- Docs: `docs/SPEC.md` (design), `docs/PLAN.md` (tasks/status), `filemap.md` (where things live).

## Conventions
- All app code in `src/` is board-agnostic and draws only through `lgfx::LGFX_Device&` from `hal.h`. Board specifics live ONLY in `src/hal/board_*.cpp` selected by build flag `BOARD_CORE2` / `BOARD_CYD`.
- Screen is 320x240 landscape by default and 240x320 when the user picks portrait in the menu (orientation stored in NVS); use `hal::width()/height()`, never hardcode 320/240 in app code.
- Battery: `hal::batteryPercent()` returns -1 when the board has none; UI hides it.
- Never commit Wi-Fi credentials; config is stored in NVS via the captive setup portal.
- `legacy/core2_original.cpp` is the pre-port reference; do not edit or build it.
- Update `filemap.md` when adding/moving files.
