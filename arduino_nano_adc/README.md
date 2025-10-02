# Arduino Nano NRF24L01 Transmitter

Arduino Nano with NRF24L01 socket adapter for wireless sensor communication.

## Hardware

- Arduino Nano (ATmega328P)
- NRF24L01+PA+LNA socket adapter (plugs into Nano headers)
- pH or TDS sensor on A0

## Quick Start

```bash
# Build and upload pH sensor
make ph PORT=/dev/ttyUSB0

# Build and upload TDS sensor
make tds PORT=/dev/ttyACM0

# Serial monitor
make monitor PORT=/dev/ttyUSB0
```

## Makefile Targets

```bash
make help      # Show all targets
make build     # Build firmware (PlatformIO)
make upload    # Upload with avrdude (faster)
make monitor   # Serial monitor
make clean     # Clean build files
make ph        # Build and upload pH sensor
make tds       # Build and upload TDS sensor
```

## Configuration

The Makefile automatically sets the sensor type:
- **pH sensor**: Uses pipe address `{'p','H','s','n','s'}`
- **TDS sensor**: Uses pipe address `{'T','D','S','s','n'}`

Both use RF channel 76 (2.476 GHz).

## Dependencies

- PlatformIO (for building and library management)
- avrdude (for fast uploading)
- minicom (for serial monitoring)

Install:
```bash
sudo apt install avrdude minicom
pip install platformio
```
