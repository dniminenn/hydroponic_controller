# Reusable Pico Libraries

This directory contains hardware driver libraries that can be reused across multiple projects.

## Libraries

### `pico_nrf24l01/`
NRF24L01 wireless transceiver driver + wrapper for Arduino Nano ADC communication.

**Files:** `nrf24l01.h`, `nrf24l01.cpp`, `nano_nrf_receiver.h`, `nano_nrf_receiver.cpp`

**Usage:**
```cpp
#include "nrf24l01.h"
#include "nano_nrf_receiver.h"

NRF24L01 nrf(spi0, CSN_PIN, CE_PIN);
nrf.init();
nrf.setChannel(76);
nrf.openReadingPipe(0, address, 5);
nrf.startListening();

NanoNRFReceiver receiver(&nrf, 0);
receiver.read();
float ph = receiver.getValue(0);
```

---

### `pico_onewire/`
1-Wire protocol implementation using PIO + DS18B20 temperature sensor driver.

**Files:** `onewire_pio.h`, `onewire_pio.cpp`, `onewire.pio`, `ds18b20.h`, `ds18b20.cpp`

**Usage:**
```cpp
#include "onewire_pio.h"
#include "ds18b20.h"
OneWirePIO ow(PIN);
DS18B20 sensor(&ow, false);
sensor.begin();
sensor.requestTemperatures();
float temp = sensor.getTempC();
```

---

### `pico_dht22/`
DHT22 (AM2302) temperature and humidity sensor driver.

**Files:** `dht22.h`, `dht22.cpp`

**Usage:**
```cpp
#include "dht22.h"
DHT22 sensor(PIN);
sensor.begin();
float temp, humidity;
sensor.readTemperatureAndHumidity(&temp, &humidity);
```

---

### `pico_sht30/`
SHT30 I2C temperature and humidity sensor driver.

**Files:** `sht30.h`, `sht30.cpp`

**Usage:**
```cpp
#include "sht30.h"
SHT30 sensor(i2c0, PIN_SDA, PIN_SCL, 0x44);
sensor.begin();
float temp, humidity;
sensor.readTemperatureAndHumidity(&temp, &humidity);
```

---

### `littlefs/`
Little FS filesystem library (external dependency).

## CMake Integration

To use these libraries in your project:

```cmake
add_executable(my_project
    main.cpp
    lib/pico_nano_adc/nano_adc.cpp
    lib/pico_dht22/dht22.cpp
    # etc...
)

target_include_directories(my_project PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_nano_adc
    ${CMAKE_CURRENT_LIST_DIR}/lib/pico_dht22
    # etc...
)
```

## License

Each library may have its own license. Check individual library directories for details.

