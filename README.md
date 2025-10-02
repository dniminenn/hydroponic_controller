# Hydroponic Controller

Pico 2 W based hydroponic system with web interface.

## Documentation

- [Arduino Nano ADC transmitter](arduino_nano_adc/README.md)
  - [Wiring](arduino_nano_adc/WIRING.md)
  - [NRF24L01 setup](arduino_nano_adc/NRF24L01_SETUP.md)
- [Test RF sketch](arduino_test_rf/README.md)
- [Libraries overview](lib/README.md)
- [Pico 2 W pinout](pico2w_pinout.txt)

## Architecture

**Dual-Core Design** - Leverages both ARM Cortex-M33 cores:
- **Core 0**: Time-critical sensor reading and control logic
- **Core 1**: Network management, web server, and TCP server

Thread-safe communication between cores via mutex-protected sensor data.

## Quick Start

```bash
make build && make flash && make upload-web
```

Then access: `http://[device-ip]/`

## Commands

```bash
make build       # Build firmware
make flash       # Flash to Pico (BOOTSEL mode)
make upload-web  # Upload web files via serial
make monitor     # Serial debug output
make help        # Show all commands
```

## Hardware

| Function | GPIO | Physical Pin | VCC    | Notes |
|----------|------|--------------|--------|-------|
| Lights   | 14   | 19           | 5V     | Relay module (active LOW)
| Pump     | 12   | 16           | 5V     | Relay module (active LOW)
| Heater   | 13   | 17           | 5V     | Relay module (active LOW)
| Fan      | 15   | 20           | 5V     | Relay module (active LOW)
| DS18B20  | 16   | 21           | 3.3V   | 1-Wire water temp sensor
| DHT22    | 17   | 22           | 3.3V   | Room air humidity/temp
| I2C SDA  | 4    | 6            | 3.3V   | SHT30 table humidity sensor
| I2C SCL  | 5    | 7            | 3.3V   | SHT30 table humidity sensor
| NRF MISO | 0    | 1            | 3.3V   | SPI0 RX (NRF24L01)
| NRF CSN  | 1    | 2            | 3.3V   | SPI0 CS (NRF24L01)
| NRF SCK  | 2    | 4            | 3.3V   | SPI0 SCK (NRF24L01)
| NRF MOSI | 3    | 5            | 3.3V   | SPI0 TX (NRF24L01)
| NRF CE   | 6    | 9            | 3.3V   | Chip Enable (NRF24L01)

Relay logic: `ACTIVE_HIGH = 0` (relays are active LOW). See `src/config.h`.

### Power and VSYS

- Input power: 5V `VCC` into `VSYS` (Pin 39) via a Schottky diode to prevent backfeed when USB is connected and to protect against reverse polarity.
- USB 5V (`VBUS`, Pin 40) is the USB input; do not tie external 5V to `VBUS`.
- Pico 2 W generates `3V3(OUT)` (Pin 36) from `VSYS` using its on-board buck regulator. Use `3V3(OUT)` for devices that require 3.3V only. All GPIO are 3.3V logic.
- Common ground required: tie external PSU GND to Pico GND.
- VCC policy: default to 3.3V for sensors (DS18B20, DHT22). Use 5V only where required (relay modules, Nano ADCs).

### I2C Sensors

**SHT30** (Table humidity sensor):
- Bus: I2C0 on `SDA=GPIO4 (Pin 6)`, `SCL=GPIO5 (Pin 7)`
- Address: 0x44
- Power: 3.3V

### Nano ADC Modules (pH and TDS) - NRF24L01 Wireless

**Two separate Arduino Nano (ATmega328P) boards**, each with NRF24L01 wireless:
- **pH Nano**: Reads pH sensor on A0, transmits via NRF24L01
- **TDS Nano**: Reads TDS sensor on A0, transmits via NRF24L01

Wireless communication:
- Module: NRF24L01+PA+LNA (long range)
- Bus: SPI0 on Pico (`MISO=GPIO0`, `MOSI=GPIO3`, `SCK=GPIO2`, `CSN=GPIO1`, `CE=GPIO6`)
- Channel: 76 (2.476 GHz)
- Power: 3.3V (VCC) - **NOT 5V tolerant!**
- Pipe addresses (from `src/config.h`):
  - pH sensor: `{'p','H','s','n','s'}`
  - TDS sensor: `{'T','D','S','s','n'}`
- Data rate: 1Mbps, 16-byte payload (4 floats)

## Flash Layout (4MB)

LittleFS stores both web assets and configuration with wear leveling.

```
┌─────────────────────────────────────┐ 0 KB
│         Program Code & Data         │
│                 1.5 MB              │
├─────────────────────────────────────┤ 1536 KB
│       LittleFS File System          │
│    Web files + config (2.5 MB)      │
├─────────────────────────────────────┤ 4096 KB (4 MB)
```

Offsets (from `src/storage/flash_storage.h`):
- `LITTLEFS_FLASH_OFFSET = 1.5 MB`
- `LITTLEFS_FLASH_SIZE   = 2.5 MB`

## Configuration

Edit `src/config.h` for WiFi credentials and settings. Defaults:
- `WIFI_SSID`, `WIFI_PASS`, `TCP_PORT=47293`, `WEB_PORT=80`
- Time: `NTP_SERVER`, `TZSTR`
- Schedules and thresholds (lights, pump, heater, humidity)

## API

- `GET /api/status` - Sensor readings
- `GET /api/config` - Current config
- `POST /api/lights` - Update lights
- `POST /api/pump` - Update pump
- `POST /api/heater` - Update heater
- `POST /api/fan` - Update fan
- `POST /api/save` - Save config

## TCP Interface (Port 47293)

```bash
# Connect via netcat
nc [device-ip] 47293

# Or with echo commands
echo "status" | nc [device-ip] 47293
```

### Commands

```
lights HH:MM HH:MM    # Set lights schedule (e.g. lights 08:30 19:45)
pump ON_SEC PERIOD    # Set pump timing (e.g. pump 60 600)
heater C              # Set heater setpoint (e.g. heater 20.5)
humidity C            # Set humidity threshold (e.g. humidity 60.0)
mode timer|humidity   # Pump mode
fan on|off            # Manual fan control
minrun SEC            # Min pump run (5-300)
minoff SEC            # Min pump off (60-3600)
maxoff SEC            # Max pump off (300-7200)
status                # Current state
temp                  # Temperature
humid                 # Humidity
save                  # Save config (LittleFS)
load                  # Load config
help                  # List commands
```

## Serial Commands

```
LIST                 # Show files in LittleFS
DELETE /app.css      # Delete file
FORMAT               # Erase/format LittleFS (careful!)
```

## Dependencies

```bash
sudo apt install cmake gcc-arm-none-eabi picotool minicom
pip3 install pyserial
```