# File map
CLAUDE.md — agent instructions (short)
filemap.md — this index
README.md — user-facing docs (features, supported boards, wiring/flash, setup, credits)
LICENSE — MIT
platformio.ini — envs: core2, cyd; lib deps pinned
docs/SPEC.md — design/architecture spec
docs/PLAN.md — task list and status
docs/BOARDS.md — per-board notes and how to add a new board
legacy/core2_original.cpp — original single-file Core2 sketch (reference only)
src/main.cpp — setup/loop, app state, touch gestures
src/hal/hal.h — board-abstraction interface
src/hal/board_core2.cpp — Core2 impl (M5Unified)
src/hal/board_cyd.cpp — CYD impl (LovyanGFX ILI9341 + XPT2046)
src/menu.h/.cpp — on-screen menu (orientation, re-run setup)
src/geo.h/.cpp — haversine, bearing
src/config.h/.cpp — NVS settings + setup web portal
src/radar.h/.cpp — tar1090 fetch + drawing
