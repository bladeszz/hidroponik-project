# Hidroponik Project

IoT-based hydroponic grow controller with data logging and remote access via Supabase.

## Overview

A professional ESP32-based controller for hydroponic systems. Monitors multiple sensors (TDS, temperature/humidity, water level), controls pump and LED via relays on a fixed schedule, and synchronizes configuration & telemetry with a Supabase backend. Features robust error handling, watchdog protection, and an intuitive WiFi onboarding portal.

**Use case:** Home gardeners, researchers, and small-scale growers who want reliable, networked control of nutrient solutions and lighting with minimal setup.

---

## Key Features

✓ **Multi-sensor support:** TDS (nutrient level), DHT22 (temperature/humidity), water level
✓ **Relay control:** Pump and LED with daily scheduling + manual override
✓ **RTC (DS1302):** Accurate timekeeping independent of WiFi
✓ **WiFi onboarding:** Captive portal (WiFiManager) for zero-touch setup
✓ **Remote management:** Supabase integration for live monitoring and command dispatch
✓ **Reliability:** Watchdog timer, non-blocking reconnection, temperature-compensated TDS
✓ **Open-source:** MIT licensed

---

## Hardware

### Bill of Materials (BOM)

| Component | Qty | Notes |
|-----------|-----|-------|
| ESP32 Dev Board | 1 | Any variant (e.g., ESP32-WROOM-32) |
| DHT22 | 1 | Temperature & humidity sensor |
| TDS Sensor (analog) | 1 | Measures nutrient concentration (0–1000 ppm typical) |
| Water Level Sensor (HW-038 or similar) | 1 | Capacitive or resistive analog |
| DS1302 RTC Module | 1 | Real-time clock; optional but recommended |
| Relay Module (2-channel) | 1 | Active-LOW relay for pump & LED control |
| Jumper wires, breadboard | – | For prototyping |
| USB cable (micro/USB-C) | 1 | For programming ESP32 |

### Typical Wiring Diagram

```
ESP32 GPIO Pins:
  ├─ Pin 0 (GPIO0)  → BOOT button (reset WiFi)
  ├─ Pin 4 (GPIO4)  → DHT22 data
  ├─ Pin 5 (GPIO5)  → Relay CH1 (pump)
  ├─ Pin 16 (GPIO16) → TDS power
  ├─ Pin 17 (GPIO17) → Water level power
  ├─ Pin 18 (GPIO18) → Relay CH2 (LED)
  ├─ Pin 25 (GPIO25) → RTC CLK
  ├─ Pin 26 (GPIO26) → RTC DAT
  ├─ Pin 27 (GPIO27) → RTC RST
  ├─ Pin 32 (GPIO32) → TDS signal (ADC1)
  └─ Pin 36 (GPIO36) → Water level signal (ADC1)
```

**See `config.h` for full pin mapping. Adapt to your hardware.**

---

## Software Requirements

### Arduino IDE / PlatformIO

1. **Board:** ESP32 board support (install via Boards Manager)
2. **Required libraries:**
   - `WiFiManager` (tzapu/WiFiManager)
   - `ArduinoJson` (bblanchon/ArduinoJson)
   - `RtcDS1302` (Makuna/Rtc)
   - `DHT sensor library` (Adafruit/DHT sensor)
   - `HTTPClient` (built-in with ESP32 core)

   Install via **Sketch > Include Library > Manage Libraries** or PlatformIO CLI.

### Supabase Setup

1. Create a free Supabase account at https://supabase.com
2. Create a new project
3. Create a `devices` table with these columns:
   ```sql
   CREATE TABLE devices (
     device_id TEXT PRIMARY KEY,
     led_start TEXT DEFAULT '08:00',
     led_end TEXT DEFAULT '20:00',
     led_override BOOLEAN,
     pump_slots TEXT DEFAULT '08:00,20:00',
     pump_override BOOLEAN,
     temperature FLOAT,
     humidity FLOAT,
     water_level INTEGER,
     tds FLOAT,
     clock_time TIMESTAMP,
     last_seen TIMESTAMP DEFAULT NOW(),
     power_status BOOLEAN,
     wifi_status BOOLEAN,
     pump_status BOOLEAN,
     led_status BOOLEAN,
     created_at TIMESTAMP DEFAULT NOW(),
     updated_at TIMESTAMP DEFAULT NOW()
   );
   ```
4. Copy your project URL and anon key from **Settings > API**

---

## Getting Started

### 1. Clone & Configure

```bash
git clone https://github.com/bladeszz/hidroponik-project.git
cd hidroponik-project
cp config.h.example config.h
```

### 2. Edit `config.h`

Fill in your Supabase credentials:
```cpp
const char* SUPABASE_URL = "https://YOUR_PROJECT_ID.supabase.co";
const char* SUPABASE_KEY = "YOUR_ANON_KEY";
```

Adjust pins if your wiring differs from the defaults.

### 3. Install Libraries

In Arduino IDE:
- Sketch > Include Library > Manage Libraries
- Search & install: WiFiManager, ArduinoJson, RtcDS1302, DHT

### 4. Upload to ESP32

1. Plug in your ESP32 via USB
2. Select **Tools > Board: "ESP32 Dev Module"**
3. Select **Tools > Port: COMx** (your device)
4. Click Upload

### 5. First Boot

- Serial monitor shows: `Hidroponik-<deviceID>` WiFi AP
- Connect to it (password: `topraksiz`)
- Browser opens auto config portal → select your WiFi, enter credentials
- Device registers itself in Supabase
- Check **Serial Monitor (115200)** for logs

---

## Usage

### Local Control

Monitor via **Serial Monitor (115200 baud)**:
```
Tarih: 08/07/2026  Saat: 09:30:15
Nem: 65.2%  Sicaklik: 24.5 C
Su Seviye: 512
TDS: 450 ppm
Pompa: ACIK  LED: ACIK  WiFi: OK
-----------------------
```

### Remote Control (Supabase)

Edit the `devices` table row for your device:
- **`led_start` / `led_end`** → Daily LED on/off times (HH:MM format)
- **`pump_slots`** → Comma-separated pump cycle start times (e.g., `"06:00,12:00,18:00"`)
- **`led_override`** → `null` (use schedule), `true` (force on), `false` (force off)
- **`pump_override`** → Same as LED

Changes sync within ~5 seconds.

### Reset WiFi

Hold the **BOOT button for 3 seconds** on the ESP32 to clear WiFi credentials and restart.

---

## Project Structure

```
├── Hidroponik.ino       Main program (setup + loop)
├── config.h             ⚠️ YOUR SECRETS (gitignored)
├── config.h.example     Template for config.h
├── globals.h            Global variables & objects
├── sensors.ino          TDS, water level, DHT22 reading
├── relays.ino           Pump & LED scheduling engine
├── supabase.ino         HTTP integration (send/receive)
├── wifi_utils.ino       Device ID, MAC, WiFi helpers
├── .gitignore           Prevents accidental secret commits
└── README.md            This file
```

---

## Security & Safety

⚠️ **Important:**

1. **Never commit `config.h`** with real credentials. Use `.gitignore` (already configured).
2. **Keep `SUPABASE_KEY` private.** Treat like a password.
3. **RTC battery:** Install a CR2032 in the DS1302 module for persistence across power cycles.
4. **High voltage:** Use proper relays/optoisolation for AC pumps/lights. Low-voltage only on GPIO pins.
5. **Watchdog:** System reboots if loop stalls > 60 seconds. Check for infinite loops in custom code.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| DHT22 not reading | Check wiring, ensure 10kΩ pullup on data line, verify pin in config.h |
| TDS always 0 | Probe inserted in water? Power pin high? Try adjusting `TDS_FACTOR` in config.h |
| WiFi won't connect | Use BOOT button to reset → reconfigure portal; check SSID/password |
| Supabase errors | Verify URL/key in config.h; check table schema; inspect serial logs |
| RTC not valid | Battery dead? Check 3V CR2032; verify wiring to pins 25/26/27 |
| Relays won't switch | Verify GPIO pins in config.h; check relay module power; test with multimeter |

---

## Performance & Limits

- **Sampling rate:** TDS filtered over 30 samples @ 40ms interval → ~1.2 sec median
- **Send interval:** Telemetry to Supabase every 10 seconds
- **WiFi timeout:** 5 minutes (AP portal closes, switches to STA+reconnect)
- **Memory:** ~40% heap used on boot, safe margin for dynamic buffers

---

## Contributing

Found a bug? Have a feature idea? Contributions welcome:

1. Fork the repo
2. Create a branch (`git checkout -b feature/my-feature`)
3. Commit changes (`git commit -am 'Add feature'`)
4. Push (`git push origin feature/my-feature`)
5. Open a Pull Request

---

## License

MIT License — see [LICENSE](LICENSE) file.

**TL;DR:** Use, modify, distribute freely. Include original copyright and license in copies.

---

## Support & Community

- **Issues:** Report bugs via GitHub Issues
- **Discussions:** Share ideas and ask questions in GitHub Discussions
- **Documentation:** Check inline code comments for implementation details

---

## Changelog

### v3.2 (Current)
- RTC-based scheduling with override support
- Supabase integration for telemetry & commands
- WiFiManager captive portal
- Watchdog timer & error recovery
- Temperature-compensated TDS

---

**Made with ❤️ for open-source IoT**

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
