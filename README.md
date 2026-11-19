# MatrixClock

A custom ESP32-S3 firmware for a 64×64 RGB LED matrix desk clock with a round rotary touchscreen menu, WiFi/NTP time sync, configurable idle power management, and a built-in captive-portal web UI.

Originally inspired by [jnthas/clockwise](https://github.com/jnthas/clockwise).

---

## Target Hardware

Built specifically for the **[Makerfabs MaTouch 1.28" Toolset with RGB LED Matrix](https://www.makerfabs.com/matouch-1-28-toolset-rgb-led-matrix.html)**:

- **MCU:** ESP32-S3 (dual-core, 16 MB Flash, OPI PSRAM, USB CDC)
- **LED Matrix:** 64×64 HUB75 RGB LED matrix panel driven via I2S DMA
- **Menu Display:** 1.28" 240×240 round IPS TFT (`GC9A01` over SPI) with LVGL UI
- **Input:** Capacitive touchscreen (`CST816S` over I2C) + quadrature rotary encoder dial
- **RTC:** `PCF8563` real-time clock over shared I2C bus (keeps time offline between NTP syncs)

---

## Controls & Features

- **Switch Clock Faces:** Turn the rotary dial to cycle through enabled faces on the round menu screen. Your last selected face is saved to NVS and restored on boot.
- **Adjust Matrix Brightness:** Swipe left on the round touchscreen to open the brightness arc, then turn the rotary dial (`0–100%`). Swipe back to return to the face menu.
- **Idle Dim & Sleep:** Automatically dims the LED matrix after the configured dim timeout (default 5 min) and turns both displays off after the sleep timeout (default 15 min). Touching the screen or turning the dial wakes the clock without accidentally changing the active face.
- **WiFi & Web Settings Portal:** Selecting **Settings** starts a temporary WiFi access point (`MatrixClock-Setup` at `http://192.168.4.1`) with a tabbed web UI to configure:
  - **WiFi:** Scan nearby networks and save/test home WiFi credentials for NTP sync.
  - **Display:** Rotate the round menu dial (`0° / 90° / 180° / 270°`) and set idle dim/off timeouts (`1–720 min`).
  - **Time:** Choose time zone and DST (with live NTP sync).
  - **Messages:** Customize the custom message face's menu label, header/footer text, and rotating sayings list.
  - **Faces:** Show or hide individual clock faces from the rotary menu (`Settings` is always enabled).

---

## Clock Faces

| Menu Name | Description |
| --- | --- |
| **Mario** | Classic Super Mario pixel-art scene where Mario jumps to hit the hour and minute `?` blocks on time updates. |
| **Pacman** | Retro Pac-Man maze animation with Pac-Man navigating around a central digital time readout. |
| **Pacman 2** | Expanded Pac-Man maze with two ghosts, power pellets with invincibility flashing, death/respawn animations, and a blinking colon clock. |
| **Pokedex** | Pokédex-styled display featuring rotating Pokémon pixel sprites alongside the current time. |
| **Maple** | Animated cat (*Maple*) walking across the scene under a large 12-hour `HH:MM AM/PM` readout. |
| **Messages** | Scrolling custom encouragements/messages with customizable menu name, top/bottom text, and sayings via the web UI. |
| **Stars** | Warp-speed starfield animation radiating from the center of the 64×64 matrix. |
| **Pipes** | Classic 1990s 3D Pipes-inspired animation with colorful turning pipes, spherical joints, and occasional teapots. |
| **Analog** | 64×64 analog watch dial with numbered hours and color-coded hour, minute, and second hands. |
| **Matrix** | Falling green Matrix digital rain with randomized columns, speeds, trail lengths, and glyph flicker. |
| **Invaders** | Autonomous Space Invaders arcade battle with an accelerating alien fleet, destructible bunkers, AI ship dodging/shooting, mystery UFO, and bottom time bar. |
| **Joke** | Daily kid-friendly joke scroller fetched over WiFi (with non-blocking background fetch and offline fallback jokes) plus a lower-left clock. |
| **Hourglass** | Three graduated hourglasses (widest **Day**, medium **Hour**, slimmest **Minute**) with grain-conserving sand physics, falling sand streams, and lower-left `HH:MM AM/PM` time. |
| **Settings** | Launches the `MatrixClock-Setup` WiFi AP and captive-portal web server for device configuration. |

---

## Building & Flashing

This repository is configured for **PlatformIO** (see [`platformio.ini`](platformio.ini)):

```bash
# Build firmware
pio run

# Build and upload to the MaTouch ESP32-S3
pio run -t upload

# Open serial monitor (115200 baud)
pio device monitor
```

### Key Dependencies (`platformio.ini`)
- `platform = espressif32@6.5.0` (`board = esp32-s3-devkitc-1`)
- `ESP32 HUB75 LED MATRIX PANEL DMA Display @ 3.0.11`
- `Arduino_GFX @ v1.3.1`
- `Adafruit GFX Library @ 1.11.11`
- `RTClib @ 2.1.4`
- `lvgl @ v8.3.11`

---

## Project Structure

```text
Clock/
  Clock.ino                  # Entry point (delegates to ClockApp FreeRTOS tasks)
  src/
    app/                     # ClockApp (FreeRTOS tasks, encoder, power state, LVGL glue)
    clockfaces/              # All 14 clock face implementations + shared drawing helpers
    core/                    # ClockfaceRegistry, CWDateTime, WifiConfig (NVS/NTP), EventBus
    hardware/                # Pin configuration, shared I2C mutex, CST816S touch driver
    fonts/ & sprites/        # Bitmap fonts and pixel-art sprites
lib/ui/                      # SquareLine Studio LVGL screens for the 1.28" round TFT
```
