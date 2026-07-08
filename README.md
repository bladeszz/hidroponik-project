# Hidroponik Project

IoT-based hydroponic grow controller with data logging and remote access.

## Overview

This project is an ESP32-based hydroponic system controller (Arduino/ESP32). It reads sensors (TDS, DHT22, water level), controls relays for pump and LED scheduling, and sends/receives configuration and telemetry from Supabase. Designed for reliable operation with watchdog protection, RTC-based scheduling, and a captive WiFi onboarding portal.

## Key Features

- Sensor support: TDS, DHT22 (temperature & humidity), water level sensor
- Relay control: pump and LED with scheduling and manual override
- RTC (DS1302) based schedule engine
- WiFi captive portal for easy onboarding (WiFiManager)
- Data logging and remote commands using Supabase
- Watchdog timer, non-blocking WiFi reconnection, and memory-conscious design

## Hardware

Minimum recommended hardware:

- ESP32 development board
- DHT22 sensor
- TDS sensor (analog)
- Water level sensor (e.g., HW-038)
- DS1302 RTC module (optional but recommended for accurate scheduling)
- Relay module for pump and LED

## Wiring (high level)

Pins are defined in `config.h`. Review and adapt them to your board and wiring before powering up.

## Software Requirements

- Arduino IDE or PlatformIO
- ESP32 board support package for Arduino
- Libraries used in the project (examples): WiFiManager, ArduinoJson, RtcDS1302, DHT, HTTPClient

Install libraries via Library Manager or PlatformIO before uploading.

## Setup

1. Open the project in Arduino IDE or PlatformIO.
2. Update `config.h`:
   - Set `SUPABASE_URL` and `SUPABASE_KEY` to your Supabase project values.
   - Adjust pin definitions if your hardware differs.
3. Compile and upload to your ESP32.
4. On first boot the device starts a WiFi configuration portal (`Hidroponik-<deviceID>`). Connect to it and configure your WiFi.
5. Device will attempt to register itself and start sending telemetry to Supabase.

## Project Structure

- `Hidroponik.ino` — Main program (setup and loop)
- `config.h` — Configuration, pin mappings, timing constants
- `globals.h` — Global variables and objects
- `sensors.ino` — Sensor reading and TDS calculations
- `relays.ino` — Relay control and scheduling logic
- `supabase.ino` — Supabase HTTP integration (register, send telemetry, fetch commands)
- `wifi_utils.ino` — Device ID, MAC printing, WiFi helpers

## Usage

- Use Supabase to configure `led_start`, `led_end`, `pump_slots` and override flags per device.
- Monitor serial output at 115200 for diagnostics.

## Troubleshooting

- If sensors are not responding check wiring and power to sensor modules.
- If WiFi does not connect, use the captive portal to set credentials or press the BOOT button for WiFi reset as described in the code comments.
- Ensure `SUPABASE_KEY` and `SUPABASE_URL` are correct and that the device can reach the internet.

## Contributing

Contributions and improvements are welcome. Please open issues for bugs or feature requests, and submit PRs for proposed changes.

## License

No license file included at this time. If you want this project to be open-source, add a LICENSE (e.g., MIT, Apache-2.0) to the repository.

---

*Initial commit: project files and README*

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>