#include "menu.h"

#include <Arduino.h>

#include "config.h"
#include "hal/hal.h"
#include "radar.h"

namespace menu {
namespace {

constexpr uint32_t DEBOUNCE_MS = 50;
constexpr uint32_t ARM_TIMEOUT_MS = 4000;
constexpr int BTN_H = 40;
constexpr int GAP = 8;
constexpr int TITLE_Y = 8;

enum Id { ORIENT0, ORIENT1, ORIENT2, ORIENT3, RERUN, BACK, COUNT, NONE = -1 };

struct Button {
  int x, y, w, h;
};

const char* const kOrientLabels[4] = {"Landscape", "Portrait", "Landscape flip", "Portrait flip"};

Button btn[COUNT];
int currentRot = 0;
bool rerunArmed = false;
uint32_t armedMs = 0;

void computeLayout() {
  const int W = hal::width(), H = hal::height();
  const int bw = (W - 3 * GAP) / 2;
  const int gridY = TITLE_Y + 16 + GAP + 4;
  for (int i = 0; i < 4; ++i) {
    btn[i] = {GAP + (i & 1) * (bw + GAP), gridY + (i >> 1) * (BTN_H + GAP), bw, BTN_H};
  }
  const int rowY = H - GAP - BTN_H;
  btn[RERUN] = {GAP, rowY, bw, BTN_H};
  btn[BACK] = {GAP + bw + GAP, rowY, bw, BTN_H};
}

int hit(int x, int y) {
  for (int i = 0; i < COUNT; ++i) {
    if (x >= btn[i].x && x < btn[i].x + btn[i].w && y >= btn[i].y && y < btn[i].y + btn[i].h) return i;
  }
  return NONE;
}

// Largest bitmap-font scale at which every label fits inside a button.
float labelScale() {
  auto& d = hal::display();
  const char* all[] = {"Landscape", "Portrait", "Landscape flip", "Portrait flip", "Re-run setup", "Tap again", "Back"};
  const float scales[] = {2.0f, 1.5f};
  for (float s : scales) {
    d.setTextSize(s);
    bool fits = true;
    for (const char* t : all) {
      if (d.textWidth(t) > btn[0].w - 8) fits = false;
    }
    if (fits) return s;
  }
  return 1.0f;
}

void drawButton(const Button& b, const char* label, uint16_t fill, uint16_t outline, uint16_t text, float scale) {
  auto& d = hal::display();
  d.fillRoundRect(b.x, b.y, b.w, b.h, 6, fill);
  d.drawRoundRect(b.x, b.y, b.w, b.h, 6, outline);
  d.drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 5, outline);
  d.setTextSize(scale);
  d.setTextDatum(lgfx::middle_center);
  d.setTextColor(text, fill);
  d.drawString(label, b.x + b.w / 2, b.y + b.h / 2);
}

void draw() {
  auto& d = hal::display();
  computeLayout();
  const uint16_t grey = d.color565(48, 48, 48);
  d.fillScreen(TFT_BLACK);
  d.setTextDatum(lgfx::top_center);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(2);
  d.drawString("Menu", hal::width() / 2, TITLE_Y);
  const float s = labelScale();
  for (int i = 0; i < 4; ++i) {
    const bool sel = (i == currentRot);
    drawButton(btn[i], kOrientLabels[i], sel ? TFT_GREEN : grey, TFT_WHITE, sel ? TFT_BLACK : TFT_WHITE, s);
  }
  if (rerunArmed) {
    drawButton(btn[RERUN], "Tap again", TFT_RED, TFT_RED, TFT_WHITE, s);
  } else {
    drawButton(btn[RERUN], "Re-run setup", grey, TFT_RED, TFT_WHITE, s);
  }
  drawButton(btn[BACK], "Back", grey, TFT_GREEN, TFT_WHITE, s);
}

void disarm() {
  if (!rerunArmed) return;
  rerunArmed = false;
  draw();
}

// Returns true when the menu should close.
bool activate(int id) {
  if (id >= ORIENT0 && id <= ORIENT3) {
    rerunArmed = false;
    if (id != currentRot) {
      currentRot = id;
      hal::setOrientation(id);
      config::saveRotation(id);
    }
    draw();  // also refreshes a cancelled "Tap again" label
    return false;
  }
  if (id == RERUN) {
    if (!rerunArmed) {
      rerunArmed = true;
      armedMs = millis();
      draw();
      return false;
    }
    config::clear();
    radar::message("Setup cleared", "Restarting...");
    delay(500);
    ESP.restart();
    return true;
  }
  if (id == BACK) {
    radar::drawShell();
    return true;
  }
  disarm();  // tap outside every button
  return false;
}

}  // namespace

void run() {
  config::Settings s;
  config::load(s);
  currentRot = s.rotation;
  rerunArmed = false;
  draw();

  // Debounced touch. Starts as "pressed" so the release of the hold that opened
  // the menu (and any finger still down) is swallowed before taps are accepted.
  bool raw = false, stable = true, armedInput = false;
  uint32_t rawSince = millis();
  int x = 0, y = 0, lastX = 0, lastY = 0, pressX = 0, pressY = 0;

  for (;;) {
    hal::update();
    const uint32_t now = millis();
    const bool down = hal::touchPoint(x, y);
    if (down) {
      lastX = x;
      lastY = y;
    }
    if (down != raw) {
      raw = down;
      rawSince = now;
      if (down) { pressX = x; pressY = y; }
    }
    if (raw != stable && now - rawSince >= DEBOUNCE_MS) {
      stable = raw;
      if (!stable) {
        if (!armedInput) {
          armedInput = true;  // finger fully lifted: accept taps from now on
        } else {
          const int pressed = hit(pressX, pressY);
          const int released = hit(lastX, lastY);
          if (pressed != NONE && pressed == released) {
            if (activate(pressed)) return;
          } else {
            disarm();
          }
        }
      }
    }
    if (rerunArmed && now - armedMs >= ARM_TIMEOUT_MS) disarm();
    delay(2);
  }
}

}  // namespace menu
