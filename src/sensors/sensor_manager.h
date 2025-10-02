#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "onewire_pio.h"
#include "ds18b20.h"
#include "sht30.h"
#include "dht22.h"
#include "pico/mutex.h"
#include "../config.h"
#include "nrf24l01.h"
#include "nano_nrf_receiver.h"

// Forward declaration for DEVICE_DISCONNECTED_C
#ifndef DEVICE_DISCONNECTED_C
#define DEVICE_DISCONNECTED_C -127.0
#endif

class SensorManager {
public:
    SensorManager();
    ~SensorManager();
    
    bool initialize();
    void readTemperature();
    void readHumidity();
    void readAirSensor();
    void readNanoADCs();
    
    // Thread-safe sensor data accessors
    float getLastTemperature() const;  // Water temp from DS18B20
    float getLastHumidity() const;     // Table humidity from SHT30
    float getLastAirTemp() const;      // Room air temp from DHT22
    float getLastAirHumidity() const;  // Room air humidity from DHT22
    float getLastPH() const;           // pH from Nano ADC
    float getLastTDS() const;          // TDS from Nano ADC
    bool isTemperatureValid() const;
    bool isHumidityValid() const;
    bool isAirTempValid() const;
    bool isAirHumidityValid() const;
    bool isPHValid() const;
    bool isTDSValid() const;
    
    // Sensor status
    bool isTemperatureSensorInitialized() const { return temp_sensor_ != nullptr; }
    bool isHumiditySensorInitialized() const { return humidity_sensor_ != nullptr; }
    bool isDHT22Initialized() const { return dht22_sensor_ != nullptr; }
    bool isNanoADCInitialized() const { return nano_ph_ != nullptr && nano_tds_ != nullptr; }
    
private:
    // Sensor objects
    OneWirePIO* one_wire_;
    DS18B20* temp_sensor_;
    SHT30* humidity_sensor_;
    DHT22* dht22_sensor_;
    NRF24L01* nrf_;
    NanoNRFReceiver* nano_ph_;
    NanoNRFReceiver* nano_tds_;
    bool sensors_initialized_;
    
    // Sensor readings (protected by mutex for multicore access)
    float last_temp_c_;           // Water temperature
    float last_humidity_;         // Table humidity
    float last_air_temp_c_;       // Room air temperature
    float last_air_humidity_;     // Room air humidity
    float last_ph_;               // pH
    float last_tds_;              // TDS
    uint32_t last_temp_read_;
    uint32_t last_humidity_read_;
    uint32_t last_air_read_;
    uint32_t last_nano_read_;
    
    // Thread safety
    mutable mutex_t sensor_mutex_;
    
    // Timing
    static const uint32_t SENSOR_INTERVAL_MS = 30000UL;
};
