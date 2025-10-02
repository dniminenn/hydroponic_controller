# NRF24L01 Test - Wiring

## Wiring Table

NRF24L01+PA+LNA socket adapter with built-in 3.3V regulator - powered from 5V.

| NRF24L01 | Arduino Nano | Physical Pin |
|----------|--------------|--------------|
| VCC      | +5V          | Pin 27       |
| GND      | GND          | Pin 29       |
| CE       | D9           | Pin 12       |
| CSN      | D10          | Pin 13       |
| SCK      | D13          | Pin 16       |
| MOSI     | D11          | Pin 14       |
| MISO     | D12          | Pin 15       |

## Notes

- **NRF24L01 Adapter**: Has built-in 3.3V regulator - connect VCC to +5V
- Both Nanos wired identically
- One compiled with RF_MODE=1 (TX), other with RF_MODE=0 (RX)