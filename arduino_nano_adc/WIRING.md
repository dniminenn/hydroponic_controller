# Arduino Nano ADC + NRF24L01 - Wiring

## NRF24L01 Socket Adapter

The NRF24L01+PA+LNA socket adapter plugs directly into Arduino Nano headers.
Has built-in 3.3V regulator - can be powered from 5V.

| NRF24L01 | Arduino Nano | Physical Pin |
|----------|--------------|--------------|
| VCC      | +5V          | Pin 27       |
| GND      | GND          | Pin 29       |
| CE       | D9           | Pin 12       |
| CSN      | D10          | Pin 13       |
| SCK      | D13          | Pin 16       |
| MOSI     | D11          | Pin 14       |
| MISO     | D12          | Pin 15       |

## pH Sensor (Build with SENSOR_TYPE_PH=1)

| pH Sensor    | Arduino Nano | Physical Pin | Notes                    |
|--------------|--------------|--------------|--------------------------|
| VCC/+        | +5V          | Pin 27       | Power supply             |
| GND/-        | GND          | Pin 29       | Ground                   |
| Signal/OUT   | A0           | Pin 19       | Analog output (0-5V)     |

**Typical pH Probe Values:**
- pH 7 (neutral): ~2.5V (ADC ~512)
- pH 4 (acidic): ~3V (ADC ~614)
- pH 10 (basic): ~2V (ADC ~410)

## TDS Sensor (Build with SENSOR_TYPE_PH=0)

| TDS Sensor   | Arduino Nano | Physical Pin | Notes                    |
|--------------|--------------|--------------|--------------------------|
| VCC/+        | +5V          | Pin 27       | Power supply             |
| GND/-        | GND          | Pin 29       | Ground                   |
| Signal/OUT   | A0           | Pin 19       | Analog output (0-5V)     |

**TDS Sensor Values:**
- 0 ppm (pure water): ~0V (ADC ~0)
- 500 ppm: ~1V (ADC ~200)
- 1000 ppm: ~2V (ADC ~400)

## Spare Analog Inputs

Available for additional sensors (currently unused):

| Pin | Arduino Nano | Physical Pin |
|-----|--------------|--------------|
| A1  | A1           | Pin 20       |
| A2  | A2           | Pin 21       |
| A3  | A3           | Pin 22       |

## Power Requirements

- **Arduino Nano**: 5V via USB or VIN (7-12V)
- **NRF24L01 Adapter**: 5V input (regulates to 3.3V internally) @ 100mA+
- **pH Sensor**: 5V @ 5-10mA
- **TDS Sensor**: 5V @ 5-10mA

**Total**: ~200mA @ 5V (can be powered from USB)

## Important Notes

1. **NRF24L01 Adapter**: Has built-in 3.3V regulator - connect VCC to +5V.
2. **Capacitor**: 10-100µF capacitor across NRF24L01 VCC/GND recommended for stability.
3. **Sensor Calibration**: pH and TDS sensors require calibration for accurate readings.
4. **Watchdog**: Device will restart after 60 consecutive failed transmissions.

## Build Commands

```bash
# pH sensor
make ph PORT=/dev/ttyUSB0

# TDS sensor  
make tds PORT=/dev/ttyUSB0
```

