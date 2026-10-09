#pragma once

#if defined(BOARD_CORE2)
#include <M5Unified.h>
#elif defined(BOARD_CYD)
#include <LovyanGFX.hpp>
#else
#error "Define BOARD_CORE2 or BOARD_CYD"
#endif

namespace hal {
void begin();
void update();
lgfx::LGFX_Device& display();
int width();
int height();
int batteryPercent();  // -1 when no battery
bool touchDown();
}  // namespace hal
