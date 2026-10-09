# Plan

Status: [ ] todo, [x] done

1. [x] **Scaffold** (Light) — `platformio.ini` (envs core2, cyd, pinned libs, build flags BOARD_CORE2/BOARD_CYD, monitor_speed 115200), `src/main.cpp` stub, `src/hal/hal.h`, empty board files. Verify: both envs compile.
2. [x] **HAL + board impls** (Careful; after 1) — implement `hal.h`, `board_core2.cpp` (M5Unified), `board_cyd.cpp` (LovyanGFX ILI9341 + XPT2046 + backlight). Verify: both compile; review pins against known CYD pinout.
3. [x] **Port app code** (Medium; after 2) — split legacy sketch into geo/config/radar/main per SPEC; replace `M5.Display` with `hal::display()`, battery/touch via hal; fix double draw; add lat/lon validation in portal. Keep behaviour identical. Verify: both envs compile warning-free.
4. [x] **Docs** (Medium; after 1, parallel with 2-3) — README.md (features, screenshots placeholder, supported boards, hardware, build/flash with PlatformIO, first-boot setup, tar1090 requirement, troubleshooting, roadmap, credits to MatixYo's ESP32-Plane-Radar), `docs/BOARDS.md` (pin tables, CYD variants/quirks, how to add a board), LICENSE (MIT, 2026, OSRdesign).
5. [x] **Review + publish** (orchestrator; after 2-4) — final adversarial review, build both envs, commit, create public GitHub repo via gh (confirm name first), push.

Notes: Verified on hardware (2-USB CYD): display uses ST7789 -> env `cyd_st7789`. Tasks 1-4 done; app firmware awaiting on-device test.
