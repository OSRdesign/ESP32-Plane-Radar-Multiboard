#!/usr/bin/env python3
"""Render README mockups of the radar and menu screens.

Reproduces the drawing code in src/radar.cpp (drawShell / drawAircraft) and src/menu.cpp
(draw) at the same pixel coordinates, using fictional sample aircraft. Output goes to
docs/images/ (each image upscaled 2x with nearest-neighbour, plus overview.png).

Text uses the classic 6x8 GLCD bitmap font that LovyanGFX draws by default. It is read from
LovyanGFX's glcdfont.h in .pio/libdeps (present after any `pio run`); without it a monospace
TrueType font is used as an approximation.

Usage:  python tools/render_mockups.py      (needs Pillow: python -m pip install pillow)
"""
import glob
import math
import os
import re

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "docs", "images")


def rgb565(v):
    r, g, b = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))


def color565(r, g, b):
    return rgb565(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))


BLACK = rgb565(0x0000)
WHITE = rgb565(0xFFFF)
RED = rgb565(0xF800)
GREEN = rgb565(0x07E0)
DARKGREEN = rgb565(0x03E0)
DARKGREY = rgb565(0x7BEF)
YELLOW = rgb565(0xFFE0)


# ---------------------------------------------------------------- font

def load_glcd_font():
    for path in glob.glob(os.path.join(ROOT, ".pio", "libdeps", "*", "LovyanGFX", "src", "lgfx",
                                       "Fonts", "glcdfont.h")):
        with open(path, encoding="utf-8", errors="replace") as f:
            src = f.read()
        m = re.search(r"font\[\]\s*PROGMEM\s*=\s*\{(.*?)\};", src, re.S)
        if m:
            body = re.sub(r"//.*", "", m.group(1))
            data = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", body)]
            if len(data) >= 128 * 5:
                return data
    return None


GLCD = load_glcd_font()
_TTF_CACHE = {}


def _ttf(size_px):
    if size_px not in _TTF_CACHE:
        font = None
        for name in ("DejaVuSansMono.ttf", "consola.ttf", "cour.ttf"):
            try:
                font = ImageFont.truetype(name, size_px)
                break
            except OSError:
                pass
        _TTF_CACHE[size_px] = font or ImageFont.load_default()
    return _TTF_CACHE[size_px]


class Screen:
    """Minimal LovyanGFX-like canvas (text datum, size, fg/bg colour)."""

    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = Image.new("RGB", (w, h), BLACK)
        self.d = ImageDraw.Draw(self.img)
        self.datum = "top_left"
        self.size = 1.0
        self.fg, self.bg = WHITE, BLACK

    # Text metrics of the 6x8 GLCD font: every glyph cell (incl. spacing column) is 6x8.
    def text_width(self, s):
        return int(len(s) * 6 * self.size)

    def font_height(self):
        return int(8 * self.size)

    def draw_string(self, s, x, y):
        w, h = self.text_width(s), self.font_height()
        horiz = self.datum.split("_")[1]
        vert = self.datum.split("_")[0]
        if horiz == "center":
            x -= w // 2
        elif horiz == "right":
            x -= w
        if vert == "middle":
            y -= h // 2
        elif vert == "bottom":
            y -= h
        sz = self.size
        for i, ch in enumerate(s):
            cx = x + int(i * 6 * sz)
            cw = int((i + 1) * 6 * sz) - int(i * 6 * sz)
            # background of the whole 6x8 cell (fg != bg in every call of the firmware)
            self.d.rectangle([cx, y, cx + cw - 1, y + h - 1], fill=self.bg)
            if GLCD is not None:
                code = ord(ch)
                for col in range(5):
                    bits = GLCD[code * 5 + col]
                    x0, x1 = cx + int(col * sz), cx + int((col + 1) * sz)
                    for row in range(8):
                        if bits & (1 << row):
                            y0, y1 = y + int(row * sz), y + int((row + 1) * sz)
                            self.d.rectangle([x0, y0, x1 - 1, y1 - 1], fill=self.fg)
            else:
                self.d.text((cx, y), ch, font=_ttf(max(8, h)), fill=self.fg)

    # Bresenham-style midpoint circle, as Adafruit_GFX / LovyanGFX drawCircle.
    def draw_circle(self, x0, y0, r, c):
        f, ddx, ddy, x, y = 1 - r, 1, -2 * r, 0, r
        pts = {(x0, y0 + r), (x0, y0 - r), (x0 + r, y0), (x0 - r, y0)}
        while x < y:
            if f >= 0:
                y -= 1
                ddy += 2
                f += ddy
            x += 1
            ddx += 2
            f += ddx
            for px, py in ((x, y), (y, x)):
                pts.update({(x0 + px, y0 + py), (x0 - px, y0 + py),
                            (x0 + px, y0 - py), (x0 - px, y0 - py)})
        self.d.point(list(pts), fill=c)

    def line(self, x0, y0, x1, y1, c):
        self.d.line([x0, y0, x1, y1], fill=c)

    def fill_triangle(self, pts, c):
        self.d.polygon(pts, fill=c, outline=c)

    def round_rect(self, x, y, w, h, r, fill=None, outline=None):
        self.d.rounded_rectangle([x, y, x + w - 1, y + h - 1], radius=r, fill=fill, outline=outline)


# ---------------------------------------------------------------- radar (src/radar.cpp)

RADIUS = 100

SAMPLE = {
    "range_km": 25,
    "total": 12,
    "online": True,
    "overhead": "TRA6231",
    # (callsign, bearing deg, distance km) - fictional
    "aircraft": [
        ("KLM1234", 35, 17.5),
        ("BAW56X", 285, 21.0),
        ("RYR8QK", 140, 12.0),
        ("DLH4AB", 75, 23.5),
        ("EZY91C", 210, 19.0),
        ("AFR7710", 330, 8.5),
        ("TRA6231", 120, 1.2),
    ],
}


def draw_shell(s, battery, data):
    cx, cy = s.w // 2, (120 if s.w > s.h else 190)
    s.datum, s.size = "top_left", 1
    line = 6
    if battery is not None and battery >= 0:
        s.fg, s.bg = WHITE, BLACK
        s.draw_string("BAT %d%%" % max(0, min(100, battery)), 6, line)
    line += 14
    s.fg = GREEN if data["online"] else RED
    s.draw_string("Receiver online" if data["online"] else "Offline", 6, line)
    line += 14
    s.fg = WHITE
    s.draw_string("Total planes: %d" % data["total"], 6, line)
    line += 14
    s.draw_string("Over head: " + data["overhead"], 6, line)
    s.draw_circle(cx, cy, RADIUS, GREEN)
    s.draw_circle(cx, cy, 75, DARKGREEN)
    s.draw_circle(cx, cy, 50, DARKGREEN)
    s.draw_circle(cx, cy, 25, DARKGREEN)
    s.line(cx - RADIUS, cy, cx + RADIUS, cy, DARKGREEN)
    s.line(cx, cy - RADIUS, cx, cy + RADIUS, DARKGREEN)
    s.datum, s.fg, s.size = "top_center", WHITE, 1
    s.draw_string("N", cx, cy - RADIUS - 11)
    s.draw_string("S", cx, cy + RADIUS + 2)
    s.datum = "middle_left"
    s.draw_string("W", cx - 109, cy)
    s.draw_string("E", cx + 104, cy)
    s.datum, s.fg = "top_right", YELLOW
    s.draw_string("%d km" % int(data["range_km"]), s.w - 10, 8)
    s.datum, s.fg = "bottom_center", DARKGREY
    s.draw_string("Tap: range  Hold 5 s: menu", cx, s.h - 2)


def draw_aircraft(s, bearing, distance, label, range_km):
    if distance > range_km:
        return
    cx, cy = s.w // 2, (120 if s.w > s.h else 190)
    a = math.radians(bearing - 90.0)
    r = distance / range_km * RADIUS
    x, y = cx + int(math.cos(a) * r), cy + int(math.sin(a) * r)
    s.fill_triangle([(x, y - 5), (x - 4, y + 4), (x + 4, y + 4)], RED)
    s.fg, s.bg, s.size, s.datum = WHITE, BLACK, 1, "top_left"
    s.draw_string(label, max(0, min(x + 6, s.w - 50)), max(0, min(y - 8, s.h - 15)))


def render_radar(w, h, battery):
    s = Screen(w, h)
    draw_shell(s, battery, SAMPLE)
    for cs, brg, dist in SAMPLE["aircraft"]:
        draw_aircraft(s, brg, dist, cs, SAMPLE["range_km"])
    return s.img


# ---------------------------------------------------------------- menu (src/menu.cpp)

BTN_H, GAP, TITLE_Y = 40, 8, 8
ORIENT_LABELS = ["Landscape", "Portrait", "Landscape flip", "Portrait flip"]


def render_menu(w, h, current_rot):
    s = Screen(w, h)
    bw = (w - 3 * GAP) // 2
    grid_y = TITLE_Y + 16 + GAP + 4
    btn = [(GAP + (i & 1) * (bw + GAP), grid_y + (i >> 1) * (BTN_H + GAP), bw, BTN_H) for i in range(4)]
    row_y = h - GAP - BTN_H
    rerun, back = (GAP, row_y, bw, BTN_H), (GAP + bw + GAP, row_y, bw, BTN_H)
    grey = color565(48, 48, 48)

    s.datum, s.fg, s.bg, s.size = "top_center", WHITE, BLACK, 2
    s.draw_string("Menu", w // 2, TITLE_Y)

    labels = ORIENT_LABELS + ["Re-run setup", "Tap again", "Back"]
    scale = 1.0
    for cand in (2.0, 1.5):
        s.size = cand
        if all(s.text_width(t) <= bw - 8 for t in labels):
            scale = cand
            break

    def button(b, label, fill, outline, text):
        x, y, bw_, bh = b
        s.round_rect(x, y, bw_, bh, 6, fill=fill)
        s.round_rect(x, y, bw_, bh, 6, outline=outline)
        s.round_rect(x + 1, y + 1, bw_ - 2, bh - 2, 5, outline=outline)
        s.size, s.datum, s.fg, s.bg = scale, "middle_center", text, fill
        s.draw_string(label, x + bw_ // 2, y + bh // 2)

    for i in range(4):
        sel = i == current_rot
        button(btn[i], ORIENT_LABELS[i], GREEN if sel else grey, WHITE, BLACK if sel else WHITE)
    button(rerun, "Re-run setup", grey, RED, WHITE)
    button(back, "Back", grey, GREEN, WHITE)
    return s.img


# ---------------------------------------------------------------- output

def main():
    os.makedirs(OUT, exist_ok=True)
    shots = [
        ("radar-landscape.png", render_radar(320, 240, 87)),   # Core2 (battery gauge)
        ("radar-portrait.png", render_radar(240, 320, None)),  # CYD (no battery)
        ("menu-landscape.png", render_menu(320, 240, 0)),
        ("menu-portrait.png", render_menu(240, 320, 1)),
    ]
    big = []
    for name, img in shots:
        up = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
        up.save(os.path.join(OUT, name))
        big.append(up)
        print("wrote", os.path.join("docs", "images", name), up.size)

    gap, bg = 24, (30, 30, 30)
    ow = sum(i.width for i in big) + gap * (len(big) + 1)
    oh = max(i.height for i in big) + gap * 2
    overview = Image.new("RGB", (ow, oh), bg)
    x = gap
    for i in big:
        overview.paste(i, (x, (oh - i.height) // 2))
        x += i.width + gap
    overview.save(os.path.join(OUT, "overview.png"))
    print("wrote", os.path.join("docs", "images", "overview.png"), overview.size)
    if GLCD is None:
        print("note: LovyanGFX glcdfont.h not found in .pio/libdeps; used a TrueType fallback font")


if __name__ == "__main__":
    main()
