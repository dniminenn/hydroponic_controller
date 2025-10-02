# NRF24L01 Test - Simple TX/RX Test

Simple test to verify NRF24L01 modules are working correctly.

## Hardware

- 2x Arduino Nano (ATmega328P)
- 2x NRF24L01+PA+LNA socket adapter (plugs into Nano headers)

## Quick Start

```bash
# Build and upload transmitter
make tx PORT=/dev/ttyUSB0

# Build and upload receiver  
make rx PORT=/dev/ttyACM0

# Monitor transmitter
make monitor PORT=/dev/ttyUSB0

# Monitor receiver (in another terminal)
make monitor PORT=/dev/ttyACM0
```

## What It Does

**Transmitter:**
- Sends test data every 1 second
- Counter, timestamp, float value, byte array
- Shows "TX #X OK" or "TX #X FAIL"

**Receiver:**
- Receives and displays all data
- Shows "RX #X from pipe Y: counter=Z float=..."
- Warns if no data for 10 seconds

## Expected Output

**Transmitter:**
```
NRF24L01 Test - TRANSMITTER Mode
Transmitter ready - Channel 76
Sending test data every 1 second...
TX #1 OK: float=3.14 time=12345
TX #2 OK: float=3.24 time=13345
```

**Receiver:**
```
NRF24L01 Test - RECEIVER Mode
Receiver ready - Channel 76
Listening for test data...
RX #1 from pipe 0: counter=1 float=3.14 time=12345 bytes=[0,1,2,3,4]
RX #2 from pipe 0: counter=2 float=3.24 time=13345 bytes=[0,1,2,3,4]
```

## Troubleshooting

- **"TX FAIL"** - Check wiring, power, antenna
- **No RX data** - Check channel, pipe addresses, distance
- **"No data received"** - Modules may be faulty or too far apart

## RF Settings

- Channel: 76 (2.476 GHz)
- Data rate: 1Mbps
- Power: Maximum (PA+LNA)
- Payload: 32 bytes
- Pipes: TEST1 (TX) → TEST2 (RX)
