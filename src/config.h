#pragma once

#include <stdint.h>
#include <stdbool.h>

// Pin mapping for Pico 2 W
enum : uint8_t {
    PIN_LIGHTS = 14,
    PIN_PUMP   = 12,
    PIN_HEATER = 13,
    PIN_FAN    = 15,
    PIN_TEMP   = 16,  // DS18B20
    PIN_SDA    = 4,   // I2C SDA (for SHT30)
    PIN_SCL    = 5,   // I2C SCL (for SHT30)
    PIN_DHT22  = 17,  // DHT22
    
    // NRF24L01 SPI pins (using SPI0)
    PIN_NRF_MISO = 0,  // SPI0 RX
    PIN_NRF_MOSI = 3,  // SPI0 TX
    PIN_NRF_SCK  = 2,  // SPI0 SCK
    PIN_NRF_CSN  = 1,  // SPI0 CS
    PIN_NRF_CE   = 6,  // Chip Enable
};

// Hardware configuration
#define ACTIVE_HIGH 0  // Relay logic (0 = active LOW, 1 = active HIGH)
#define EEPROM_MAGIC 0x48594453  // "HYDS" magic number for config validation

// DS18B20 power mode (0 = external power, 1 = parasitic power)
#define DS18B20_PARASITIC_POWER 0

// Nano ADC Configuration (NRF24L01 wireless)
#define NANO_ADC_ENABLED 1
#define NRF_CHANNEL 76      // RF channel (0-125)
#define NRF_PAYLOAD_SIZE 16 // 4 floats = 16 bytes

// NRF24L01 Pipe addresses (5 bytes each)
static const uint8_t NRF_ADDR_PH[5]  = {'p', 'H', 's', 'n', 's'};  // pH sensor
static const uint8_t NRF_ADDR_TDS[5] = {'T', 'D', 'S', 's', 'n'};  // TDS sensor

// WiFi and Network Configuration
#define WIFI_SSID "Legs"
#define WIFI_PASS "garfield"
#define TCP_PORT 47293
#define WEB_PORT 80
#define NTP_SERVER "192.168.0.1"
#define TZSTR "AST4ADT,M3.2.0,M11.1.0"  // POSIX timezone string

// Schedule defaults
#define DEFAULT_LIGHTS_START_S (8 * 3600)   // 08:00
#define DEFAULT_LIGHTS_END_S   (20 * 3600)  // 20:00
#define DEFAULT_PUMP_ON_SEC    45
#define DEFAULT_PUMP_PERIOD    (10 * 60)    // 10 minutes
#define DEFAULT_HEATER_SETPOINT_C 15.0
#define DEFAULT_HUMIDITY_THRESHOLD 60.0
#define DEFAULT_FAN_ON_TEMP_C  24.0
#define DEFAULT_FAN_OFF_TEMP_C 15.0

// Timing constants
#define SENSOR_READ_INTERVAL_MS 30000UL
#define STATUS_INTERVAL_MS 5000UL
#define HEATER_HYST_C 0.5f

// Flash storage configuration
struct Config {
    uint32_t magic;
    uint32_t lights_start_s;
    uint32_t lights_end_s;
    uint32_t pump_on_sec;
    uint32_t pump_period;
    float heater_setpoint_c;
    float humidity_threshold;
    bool humidity_mode;
    uint32_t min_pump_run_sec;
    uint32_t min_pump_off_sec;
    uint32_t max_pump_off_sec;
};

// Configuration manager class
class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // Configuration accessors
    uint32_t getLightsStartS() const { return lights_start_s_; }
    uint32_t getLightsEndS() const { return lights_end_s_; }
    uint32_t getPumpOnSec() const { return pump_on_sec_; }
    uint32_t getPumpPeriod() const { return pump_period_; }
    float getHeaterSetpointC() const { return heater_setpoint_c_; }
    float getHumidityThreshold() const { return humidity_threshold_; }
    bool getHumidityMode() const { return humidity_mode_; }
    uint32_t getMinPumpRunSec() const { return min_pump_run_sec_; }
    uint32_t getMinPumpOffSec() const { return min_pump_off_sec_; }
    uint32_t getMaxPumpOffSec() const { return max_pump_off_sec_; }
    
    // Configuration setters
    void setLightsStartS(uint32_t value) { lights_start_s_ = value; }
    void setLightsEndS(uint32_t value) { lights_end_s_ = value; }
    void setPumpOnSec(uint32_t value) { pump_on_sec_ = value; }
    void setPumpPeriod(uint32_t value) { pump_period_ = value; }
    void setHeaterSetpointC(float value) { heater_setpoint_c_ = value; }
    void setHumidityThreshold(float value) { humidity_threshold_ = value; }
    void setHumidityMode(bool value) { humidity_mode_ = value; }
    void setMinPumpRunSec(uint32_t value) { min_pump_run_sec_ = value; }
    void setMinPumpOffSec(uint32_t value) { min_pump_off_sec_ = value; }
    void setMaxPumpOffSec(uint32_t value) { max_pump_off_sec_ = value; }
    
    // Storage operations
    void saveConfig();
    bool loadConfig();
    void resetToDefaults();
    
private:
    ConfigManager();
    
    // Configuration values
    uint32_t lights_start_s_;
    uint32_t lights_end_s_;
    uint32_t pump_on_sec_;
    uint32_t pump_period_;
    float heater_setpoint_c_;
    float humidity_threshold_;
    bool humidity_mode_;
    uint32_t min_pump_run_sec_;
    uint32_t min_pump_off_sec_;
    uint32_t max_pump_off_sec_;
};
