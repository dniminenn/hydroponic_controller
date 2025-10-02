#include "sensor_manager.h"
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include <stdio.h>

SensorManager::SensorManager() 
    : one_wire_(nullptr), temp_sensor_(nullptr), humidity_sensor_(nullptr),
      dht22_sensor_(nullptr), nrf_(nullptr), nano_ph_(nullptr), nano_tds_(nullptr),
      sensors_initialized_(false), last_temp_c_(-999.0), 
      last_humidity_(-999.0), last_air_temp_c_(-999.0), last_air_humidity_(-999.0),
      last_ph_(-999.0), last_tds_(-999.0),
      last_temp_read_(0), last_humidity_read_(0), last_air_read_(0), last_nano_read_(0) {
    mutex_init(&sensor_mutex_);
}

SensorManager::~SensorManager() {
    if (temp_sensor_) {
        delete temp_sensor_;
        temp_sensor_ = nullptr;
    }
    if (humidity_sensor_) {
        delete humidity_sensor_;
        humidity_sensor_ = nullptr;
    }
    if (dht22_sensor_) {
        delete dht22_sensor_;
        dht22_sensor_ = nullptr;
    }
    if (nano_ph_) {
        delete nano_ph_;
        nano_ph_ = nullptr;
    }
    if (nano_tds_) {
        delete nano_tds_;
        nano_tds_ = nullptr;
    }
    if (nrf_) {
        delete nrf_;
        nrf_ = nullptr;
    }
    if (one_wire_) {
        delete one_wire_;
        one_wire_ = nullptr;
    }
}

bool SensorManager::initialize() {
    printf("Initializing sensors...\n");
    
    // Initialize OneWire for DS18B20 temperature sensor
    one_wire_ = new OneWirePIO(PIN_TEMP);
    temp_sensor_ = new DS18B20(one_wire_, DS18B20_PARASITIC_POWER);
    
    if (temp_sensor_->begin()) {
        printf("DS18B20: Temperature sensor initialized\n");
    } else {
        printf("DS18B20: Temperature sensor initialization failed\n");
        delete temp_sensor_;
        temp_sensor_ = nullptr;
    }
    
    // Initialize I2C for SHT30 humidity sensor
    humidity_sensor_ = new SHT30(i2c0, PIN_SDA, PIN_SCL, 0x44);
    
    if (humidity_sensor_->begin()) {
        printf("SHT30: Humidity sensor initialized\n");
    } else {
        printf("SHT30: Humidity sensor initialization failed\n");
        delete humidity_sensor_;
        humidity_sensor_ = nullptr;
    }
    
    // Initialize DHT22 sensor
    dht22_sensor_ = new DHT22(PIN_DHT22);
    
    if (dht22_sensor_->begin()) {
        printf("DHT22: Sensor initialized\n");
    } else {
        printf("DHT22: Sensor initialization failed\n");
        delete dht22_sensor_;
        dht22_sensor_ = nullptr;
    }
    
    // Initialize Nano ADC sensors (NRF24L01 wireless)
#if NANO_ADC_ENABLED
    spi_init(spi0, 10000000);  // 10MHz SPI
    gpio_set_function(PIN_NRF_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_NRF_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_NRF_SCK, GPIO_FUNC_SPI);
    
    nrf_ = new NRF24L01(spi0, PIN_NRF_CSN, PIN_NRF_CE);
    if (nrf_->init()) {
        nrf_->setChannel(NRF_CHANNEL);
        nrf_->setDataRate(DataRate::DR_1MBPS);
        nrf_->setPowerLevel(PowerLevel::PWR_HIGH);
        
        // Open reading pipes for pH and TDS
        nrf_->openReadingPipe(0, NRF_ADDR_PH, 5);
        nrf_->openReadingPipe(1, NRF_ADDR_TDS, 5);
        nrf_->startListening();
        
        nano_ph_ = new NanoNRFReceiver(nrf_, 0);
        nano_tds_ = new NanoNRFReceiver(nrf_, 1);
        
        printf("NRF24L01: Initialized on channel %d, listening for pH and TDS sensors\n", NRF_CHANNEL);
    } else {
        printf("NRF24L01: Initialization failed\n");
    }
#endif
    
    sensors_initialized_ = true;
    printf("Sensor initialization complete\n");
    return true;
}

void SensorManager::readTemperature() {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_temp_read_ < SENSOR_INTERVAL_MS) return;
    
    if (sensors_initialized_ && temp_sensor_) {
        if (!temp_sensor_->requestTemperatures()) {
            mutex_enter_blocking(&sensor_mutex_);
            if (last_temp_c_ > -100.0) {
                printf("Temperature sensor request failed!\n");
                last_temp_c_ = -999.0;
            }
            mutex_exit(&sensor_mutex_);
            last_temp_read_ = now;
            return;
        }
        
        // Wait for conversion (max 750ms for 12-bit resolution)
        // This is acceptable since we read every 30 seconds
        temp_sensor_->blockTillConversionComplete();
        float tempC = temp_sensor_->getTempC();
        
        if (tempC != DEVICE_DISCONNECTED_C && tempC > -50.0 && tempC < 80.0) {
            mutex_enter_blocking(&sensor_mutex_);
            last_temp_c_ = tempC;
            mutex_exit(&sensor_mutex_);
            printf("Temperature: %.2f°C\n", tempC);
        } else {
            mutex_enter_blocking(&sensor_mutex_);
            if (last_temp_c_ > -100.0) {
                printf("Temperature sensor error!\n");
                last_temp_c_ = -999.0;
            }
            mutex_exit(&sensor_mutex_);
        }
    } else {
        mutex_enter_blocking(&sensor_mutex_);
        if (last_temp_c_ > -100.0) {
            printf("Temperature sensor not initialized!\n");
            last_temp_c_ = -999.0;
        }
        mutex_exit(&sensor_mutex_);
    }
    
    last_temp_read_ = now;
}

void SensorManager::readHumidity() {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_humidity_read_ < SENSOR_INTERVAL_MS) return;
    
    if (sensors_initialized_ && humidity_sensor_) {
        float temp, humidity;
        if (humidity_sensor_->readTemperatureAndHumidity(&temp, &humidity)) {
            mutex_enter_blocking(&sensor_mutex_);
            last_humidity_ = humidity;
            mutex_exit(&sensor_mutex_);
            printf("Table Humidity (SHT30): %.2f%%\n", last_humidity_);
        } else {
            mutex_enter_blocking(&sensor_mutex_);
            if (last_humidity_ > -100.0) {
                printf("Table humidity sensor error!\n");
                last_humidity_ = -999.0;
            }
            mutex_exit(&sensor_mutex_);
        }
    } else {
        mutex_enter_blocking(&sensor_mutex_);
        if (last_humidity_ > -100.0) {
            printf("Table humidity sensor not initialized!\n");
            last_humidity_ = -999.0;
        }
        mutex_exit(&sensor_mutex_);
    }
    
    last_humidity_read_ = now;
}

void SensorManager::readAirSensor() {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_air_read_ < SENSOR_INTERVAL_MS) return;
    
    if (sensors_initialized_ && dht22_sensor_) {
        float temp, humidity;
        if (dht22_sensor_->readTemperatureAndHumidity(&temp, &humidity)) {
            mutex_enter_blocking(&sensor_mutex_);
            last_air_temp_c_ = temp;
            last_air_humidity_ = humidity;
            mutex_exit(&sensor_mutex_);
            printf("Room Air (DHT22): %.2f°C, %.2f%% RH\n", temp, humidity);
        } else {
            mutex_enter_blocking(&sensor_mutex_);
            if (last_air_temp_c_ > -100.0 || last_air_humidity_ > -100.0) {
                printf("Room air sensor error!\n");
                last_air_temp_c_ = -999.0;
                last_air_humidity_ = -999.0;
            }
            mutex_exit(&sensor_mutex_);
        }
    } else {
        mutex_enter_blocking(&sensor_mutex_);
        if (last_air_temp_c_ > -100.0 || last_air_humidity_ > -100.0) {
            printf("Room air sensor not initialized!\n");
            last_air_temp_c_ = -999.0;
            last_air_humidity_ = -999.0;
        }
        mutex_exit(&sensor_mutex_);
    }
    
    last_air_read_ = now;
}


// Thread-safe accessor implementations
float SensorManager::getLastTemperature() const {
    mutex_enter_blocking(&sensor_mutex_);
    float temp = last_temp_c_;
    mutex_exit(&sensor_mutex_);
    return temp;
}

float SensorManager::getLastHumidity() const {
    mutex_enter_blocking(&sensor_mutex_);
    float humidity = last_humidity_;
    mutex_exit(&sensor_mutex_);
    return humidity;
}

bool SensorManager::isTemperatureValid() const {
    mutex_enter_blocking(&sensor_mutex_);
    bool valid = last_temp_c_ > -100.0;
    mutex_exit(&sensor_mutex_);
    return valid;
}

bool SensorManager::isHumidityValid() const {
    mutex_enter_blocking(&sensor_mutex_);
    bool valid = last_humidity_ > -100.0;
    mutex_exit(&sensor_mutex_);
    return valid;
}

float SensorManager::getLastAirTemp() const {
    mutex_enter_blocking(&sensor_mutex_);
    float temp = last_air_temp_c_;
    mutex_exit(&sensor_mutex_);
    return temp;
}

float SensorManager::getLastAirHumidity() const {
    mutex_enter_blocking(&sensor_mutex_);
    float humidity = last_air_humidity_;
    mutex_exit(&sensor_mutex_);
    return humidity;
}

bool SensorManager::isAirTempValid() const {
    mutex_enter_blocking(&sensor_mutex_);
    bool valid = last_air_temp_c_ > -100.0;
    mutex_exit(&sensor_mutex_);
    return valid;
}

bool SensorManager::isAirHumidityValid() const {
    mutex_enter_blocking(&sensor_mutex_);
    bool valid = last_air_humidity_ > -100.0;
    mutex_exit(&sensor_mutex_);
    return valid;
}

void SensorManager::readNanoADCs() {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_nano_read_ < SENSOR_INTERVAL_MS) return;
    
#if NANO_ADC_ENABLED
    if (sensors_initialized_ && nano_ph_ && nano_tds_) {
        // Read pH
        if (nano_ph_->read()) {
            float ph = nano_ph_->getValue(0);  // A0
            if (ph > 0.0 && ph < 14.0) {
                mutex_enter_blocking(&sensor_mutex_);
                last_ph_ = ph;
                mutex_exit(&sensor_mutex_);
                printf("pH: %.2f\n", ph);
            }
        }
        
        // Read TDS
        if (nano_tds_->read()) {
            float tds = nano_tds_->getValue(0);  // A0
            if (tds >= 0.0) {
                mutex_enter_blocking(&sensor_mutex_);
                last_tds_ = tds;
                mutex_exit(&sensor_mutex_);
                printf("TDS: %.0f ppm\n", tds);
            }
        }
    }
#endif
    
    last_nano_read_ = now;
}

float SensorManager::getLastPH() const {
    mutex_enter_blocking(&sensor_mutex_);
    float ph = last_ph_;
    mutex_exit(&sensor_mutex_);
    return ph;
}

float SensorManager::getLastTDS() const {
    mutex_enter_blocking(&sensor_mutex_);
    float tds = last_tds_;
    mutex_exit(&sensor_mutex_);
    return tds;
}

bool SensorManager::isPHValid() const {
    mutex_enter_blocking(&sensor_mutex_);
    bool valid = last_ph_ > -100.0;
    mutex_exit(&sensor_mutex_);
    return valid;
}

bool SensorManager::isTDSValid() const {
    mutex_enter_blocking(&sensor_mutex_);
    bool valid = last_tds_ > -100.0;
    mutex_exit(&sensor_mutex_);
    return valid;
}
