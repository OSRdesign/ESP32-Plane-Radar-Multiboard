# ESP32 Plane Radar (multi-board)

A live ADS-B radar for ESP32 touchscreen boards. The device joins your Wi-Fi, polls a local
[tar1090](https://github.com/wiedehopf/tar1090)/readsb receiver every 5 seconds, and plots every
aircraft with a position on a round radar display centred on a latitude/longitude you choose. It
runs on the M5Stack Core2 and on the cheap "Cheap Yellow Display" (CYD) ESP32-2432S028 boards from
one PlatformIO project.

## Screenshots

> **Placeholder:** no screenshots yet. Photos of the radar running on each board will go in
> [`docs/images/`](docs/images/).

## Features

- Radar plot with range rings and N/E/S/W markers; each aircraft is drawn as a red triangle with its
  callsign (or ICAO hex code when there is no callsign).
- Four ranges: 10, 25, 50 and 100 km. Tap the screen to cycle through them.
- Status lines: receiver online/offline, total aircraft reported by the receiver, and an
  "Over head" callsign for the nearest aircraft within 2 km of the radar centre.
- Battery percentage on boards that have a battery gauge (Core2); hidden on boards without one (CYD).
- Browser-based first-boot setup through a Wi-Fi access point (no code changes or credentials in the
  source). Settings are stored in the ESP32's flash (NVS).
- Hold the screen for 5 seconds (then release) to open an on-screen menu: pick the screen orientation
  (landscape, portrait, or either flipped; remembered across reboots) or re-run the setup.
- Works fully on your local network: no cloud service or API key.

## Supported boards

| Board | Display / touch | PlatformIO env | Notes |
|---|---|---|---|
| M5Stack Core2 | 2.0" 320x240 IPS, capacitive touch | `core2` | Battery level shown. |
| Cheap Yellow Display ESP32-2432S028 / "2435S028" | 2.8" 320x240 ILI9341, XPT2046 resistive touch | `cyd` | Standard ILI9341 board. |
| CYD with inverted colours | as above | `cyd_invert` | Use if colours look wrong (e.g. black background shows white). |
| CYD with ST7789 panel | 2.8" 320x240 ST7789 | `cyd_st7789` | Use if the `cyd` build shows a mirrored or wrongly oriented image (common on the 2-USB-port boards; verified on hardware). |

Pin details and known hardware variants are in [docs/BOARDS.md](docs/BOARDS.md).

## Requirements

- One of the boards above and a USB data cable.
- An ADS-B receiver on the same local network running tar1090 (for example a Raspberry Pi with
  readsb + tar1090, or an ADS-B feeder image that includes tar1090). It must serve plain HTTP at
  `http://<receiver-ip>/tar1090/data/aircraft.json`. Open that URL in a browser first to check it
  returns JSON.
- A 2.4 GHz Wi-Fi network (the ESP32 does not support 5 GHz).
- [PlatformIO](https://platformio.org/) (the VS Code extension or the `pio` command-line tool).
  Arduino IDE is not supported.

## Build and flash

```sh
git clone https://github.com/OSRdesign/ESP32-Plane-Radar-Multiboard.git
cd ESP32-Plane-Radar-Multiboard

# Cheap Yellow Display
pio run -e cyd -t upload

# M5Stack Core2
pio run -e core2 -t upload
```

Use `cyd_invert` or `cyd_st7789` instead of `cyd` if needed (see [Troubleshooting](#troubleshooting)).
To build without flashing, drop `-t upload`. To watch the serial log, run
`pio device monitor` (115200 baud).

On Windows, Git must be installed and on your `PATH`; PlatformIO uses it to fetch libraries and the
build fails without it.

## First-boot setup

1. Power the board. With no saved settings it starts a Wi-Fi access point and shows
   **PlaneRadar-Setup**.
2. On a phone or computer, join the open Wi-Fi network `PlaneRadar-Setup`.
3. Open `http://192.168.4.1` in a browser.
4. Fill in the form:
   - **Wi-Fi name** and **Wi-Fi password**: the network your receiver is on.
   - **Receiver IP address**: the local IP of your tar1090 receiver, e.g. `192.168.1.50`
     (a leading `http://` or a trailing path is stripped automatically).
   - **Latitude** and **Longitude**: the centre of the radar in decimal degrees,
     e.g. `52.3676` and `4.9041`.
5. Press **Save and connect**. The board restarts, connects to your Wi-Fi and starts plotting.

If the board cannot connect to the saved Wi-Fi within about 15 seconds, it reopens the setup portal.

## Usage

- **Tap** the screen: cycle the range 10 → 25 → 50 → 100 → 10 km. The current range is shown at the
  top right, and the aircraft list refreshes immediately.
- **Hold** the screen for 5 seconds, then release: opens the menu.
  - **Landscape / Portrait / Landscape flip / Portrait flip**: tap one to rotate the screen
    immediately; the choice is saved and used on the next boot. The radar works in all four.
  - **Re-run setup**: tap twice (the first tap turns it into a red "Tap again"; any other tap or
    4 seconds cancels). Clears all settings, including the orientation, and restarts into the setup
    portal.
  - **Back**: return to the radar.
- **Over head**: shows the callsign of the nearest aircraft within 2 km of the radar centre, or `---`
  when there is none.
- The radar refreshes every 5 seconds.

## Troubleshooting

| Symptom | Fix |
|---|---|
| Colours are wrong (inverted, e.g. white background) on a CYD | Flash the `cyd_invert` env. |
| Screen is mirrored, rotated, blank or shows noise on a CYD | Flash the `cyd_st7789` env (ST7789 panel). If it is still dark, your board may drive the backlight on GPIO27 instead of GPIO21; see [docs/BOARDS.md](docs/BOARDS.md). |
| Taps or holds are not detected on a CYD | Press firmly (resistive touch) and check the touch pins in [docs/BOARDS.md](docs/BOARDS.md). |
| Menu buttons react at the wrong spot (mirrored or rotated) on a CYD | The touch axes differ on your board variant; see the touch note in [docs/BOARDS.md](docs/BOARDS.md). |
| Status shows **Offline** | The receiver did not answer with valid JSON. Check the IP, that `http://<ip>/tar1090/data/aircraft.json` opens in a browser from the same network, and that the receiver does not require HTTPS. Re-enter the IP via the menu (hold the screen 5 seconds, release, **Re-run setup**). |
| Setup portal keeps coming back | The board cannot join your Wi-Fi: check the name/password and that the network is 2.4 GHz. |
| No aircraft shown but receiver online | No aircraft within the current range: tap to increase it, and check the latitude/longitude. |
| Build fails on Windows fetching libraries | Install Git and make sure it is on `PATH`. |

## Project layout

```
platformio.ini           build environments (core2, cyd, ...) and pinned libraries
src/main.cpp             setup/loop, app state, touch gestures (tap / 5 s hold)
src/menu.*               on-screen menu (orientation, re-run setup)
src/hal/hal.h            board-abstraction interface (display, touch, battery)
src/hal/board_core2.cpp  M5Stack Core2 implementation (M5Unified)
src/hal/board_cyd.cpp    Cheap Yellow Display implementation (LovyanGFX)
src/geo.*                distance and bearing maths
src/config.*             settings in NVS and the setup web portal
src/radar.*              tar1090 fetch and radar drawing
docs/BOARDS.md           per-board pins, variants, adding a new board
legacy/                  original single-file Core2 sketch (reference only)
```

## Roadmap

Ideas only; none of these are implemented yet.

- Altitude and speed labels next to each aircraft.
- OpenSky Network as a fallback data source when no local receiver is available.
- More boards, e.g. other CYD variants and the M5StickC.

## Credits

Inspired by [MatixYo's ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar).
Built with [LovyanGFX](https://github.com/lovyan03/LovyanGFX),
[M5Unified](https://github.com/m5stack/M5Unified) and
[ArduinoJson](https://arduinojson.org/).

## License

MIT; see [LICENSE](LICENSE).
