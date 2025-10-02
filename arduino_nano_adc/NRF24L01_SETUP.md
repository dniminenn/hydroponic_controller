# NRF24L01 Wireless Setup Guide

## Overview

This guide explains how to set up wireless communication between Arduino Nanos (transmitters) and the Pico 2 W (receiver) using NRF24L01+PA+LNA modules.

## Hardware Requirements

### Per Nano Transmitter:
- Arduino Nano
- NRF24L01+PA+LNA module with socket adapter board (all-in-one unit with 3.3V regulator)
- pH or TDS sensor

### Pico Receiver:
- Raspberry Pi Pico 2 W
- NRF24L01+PA+LNA module (individual module)
- 10µF capacitor (recommended across VCC/GND)

## Wiring

### Arduino Nano - NRF24L01 Socket Adapter

The NRF24L01 module comes on a socket adapter board that plugs directly into the Arduino Nano headers.

**Socket Adapter Features:**
- NRF24L01+PA+LNA pre-mounted
- Built-in 3.3V regulator
- Plugs directly into Arduino pin headers

**Pin Mapping** (handled by adapter board):
- CE  → D9
- CSN → D10
- SCK → D13
- MOSI → D11
- MISO → D12
- VCC → 5V (regulated to 3.3V on adapter)
- GND → GND

**Installation:**
Simply plug the adapter board into the Nano's headers - the adapter maps all pins automatically.

### Pico - NRF24L01 Individual Module

The Pico uses a standalone NRF24L01 module (not on adapter board).

Already configured in firmware (see `src/config.h`):

| NRF24L01 Pin | Pico GPIO | Pico Physical Pin |
|--------------|-----------|-------------------|
| VCC | 3V3(OUT) | Pin 36 |
| GND | GND | Pin 38 |
| CE | GPIO6 | Pin 9 |
| CSN | GPIO1 | Pin 2 |
| SCK | GPIO2 | Pin 4 |
| MOSI | GPIO3 | Pin 5 |
| MISO | GPIO0 | Pin 1 |

**Recommendation:** Add 10µF capacitor between VCC and GND at the NRF module for power stability.

## Software Setup

### Arduino Nano (Transmitter)

1. **Install RF24 Library**:
   - Arduino IDE: `Sketch > Include Library > Manage Libraries`
   - Search for "RF24" by TMRh20
   - Install latest version

2. **Upload Sketch**:
   - Open `arduino_nano_nrf_transmitter.ino`
   - Set `SENSOR_TYPE_PH` to `1` for pH sensor, `0` for TDS sensor
   - Upload to Nano

3. **Configuration**:
   ```cpp
   #define SENSOR_TYPE_PH  1   // 1 = pH, 0 = TDS
   #define RF_CHANNEL 76       // Must match Pico
   ```

### Pico 2 W (Receiver)

Already configured in firmware (`src/config.h`):
```cpp
#define USE_NRF24L01 1      // Enable NRF24L01
#define NRF_CHANNEL 76      // RF channel
#define NRF_PAYLOAD_SIZE 16 // 4 floats
```

Pipe addresses (must match transmitters):
```cpp
static const uint8_t NRF_ADDR_PH[5]  = {'p', 'H', 's', 'n', 's'};
static const uint8_t NRF_ADDR_TDS[5] = {'T', 'D', 'S', 's', 'n'};
```

## Testing

### Test Nano Transmitter

1. Connect Nano to USB
2. Open Serial Monitor (115200 baud)
3. You should see:
   ```
   NRF24L01 Transmitter Starting...
   Configured as pH sensor
   Ready to transmit
   Channel: 76
   TX OK: 7.23, 0.00, 0.00, 0.00
   ```

### Test Pico Receiver

1. Build and flash Pico firmware
2. Connect to serial monitor
3. You should see:
   ```
   NRF24L01: Initialized on channel 76, listening for pH and TDS sensors
   pH: 7.23
   TDS: 245 ppm
   ```

## Troubleshooting

### "TX FAIL" on Nano
- Check wiring (especially VCC = 3.3V)
- Verify capacitor is installed
- Check antenna connection on PA+LNA module
- Reduce distance between modules
- Check channel matches on both sides

### No data on Pico
- Verify NRF24L01 initialized (check serial output)
- Confirm pipe addresses match
- Check channel number (must be identical)
- Verify both modules on same data rate (1Mbps)

### Intermittent Communication
- Add or upgrade capacitor (10µF minimum)
- Check power supply quality
- Reduce distance or add antenna
- Check for interference (WiFi, Bluetooth)

## RF Channel Selection

Default: Channel 76 (2.476 GHz)

To change channel (0-125):
1. Update `RF_CHANNEL` in Nano sketch
2. Update `NRF_CHANNEL` in `src/config.h`
3. Rebuild both

**Avoid WiFi channels**: Channels 0-25 (2.4-2.425 GHz) overlap with WiFi.

## Power Consumption

- NRF24L01+PA+LNA: ~115mA TX, ~45mA RX
- Arduino Nano: ~20mA
- Total per transmitter: ~135mA average

The socket adapter's 3.3V regulator handles the NRF power requirements.

## Range

- Indoor: 100-300m (line of sight)
- Outdoor: 500-1000m (line of sight)
- Through walls: 20-50m

PA+LNA modules have much better range than standard NRF24L01.

