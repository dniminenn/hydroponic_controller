#include "config.h"
#include "storage/flash_storage.h"
#include <stdio.h>
#include <string.h>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    resetToDefaults();
}

void ConfigManager::resetToDefaults() {
    lights_start_s_ = DEFAULT_LIGHTS_START_S;
    lights_end_s_ = DEFAULT_LIGHTS_END_S;
    pump_on_sec_ = DEFAULT_PUMP_ON_SEC;
    pump_period_ = DEFAULT_PUMP_PERIOD;
    heater_setpoint_c_ = DEFAULT_HEATER_SETPOINT_C;
    humidity_threshold_ = DEFAULT_HUMIDITY_THRESHOLD;
    humidity_mode_ = false;
    min_pump_run_sec_ = 30;
    min_pump_off_sec_ = 600;
    max_pump_off_sec_ = 3600;
}

void ConfigManager::saveConfig() {
    Config config;
    config.magic = EEPROM_MAGIC;
    config.lights_start_s = lights_start_s_;
    config.lights_end_s = lights_end_s_;
    config.pump_on_sec = pump_on_sec_;
    config.pump_period = pump_period_;
    config.heater_setpoint_c = heater_setpoint_c_;
    config.humidity_threshold = humidity_threshold_;
    config.humidity_mode = humidity_mode_;
    config.min_pump_run_sec = min_pump_run_sec_;
    config.min_pump_off_sec = min_pump_off_sec_;
    config.max_pump_off_sec = max_pump_off_sec_;
    
    // Store config in LittleFS for wear leveling
    FlashStorage& fs = FlashStorage::getInstance();
    if (!fs.uploadFile("/config.bin", (const uint8_t*)&config, sizeof(Config))) {
        printf("Failed to save config to LittleFS\n");
        return;
    }
    
    printf("Config saved to LittleFS\n");
}

bool ConfigManager::loadConfig() {
    // Load config from LittleFS
    FlashStorage& fs = FlashStorage::getInstance();
    uint8_t* data = nullptr;
    uint32_t size = 0;
    
    if (!fs.getFile("/config.bin", &data, &size, nullptr)) {
        printf("No valid config in LittleFS\n");
        return false;
    }
    
    if (size != sizeof(Config)) {
        printf("Config size mismatch: %u vs %u\n", size, sizeof(Config));
        fs.freeFile(data);
        return false;
    }
    
    const Config* config = (const Config*)data;
    
    // Check magic number
    if (config->magic != EEPROM_MAGIC) {
        printf("Invalid config magic\n");
        fs.freeFile(data);
        return false;
    }
    
    // Validate and load lights schedule (seconds in a day: 0-86399)
    if (config->lights_start_s < 86400 && config->lights_end_s < 86400) {
        lights_start_s_ = config->lights_start_s;
        lights_end_s_ = config->lights_end_s;
    }
    
    // Validate and load pump timing
    if (config->pump_on_sec > 0 && config->pump_on_sec < config->pump_period && 
        config->pump_period <= 7200) {  // Max 2 hour period
        pump_on_sec_ = config->pump_on_sec;
        pump_period_ = config->pump_period;
    }
    
    if (config->heater_setpoint_c > -40 && config->heater_setpoint_c < 80) {
        heater_setpoint_c_ = config->heater_setpoint_c;
    }
    
    if (config->humidity_threshold >= 0 && config->humidity_threshold <= 100) {
        humidity_threshold_ = config->humidity_threshold;
    }
    
    humidity_mode_ = config->humidity_mode;
    
    if (config->min_pump_run_sec >= 5 && config->min_pump_run_sec <= 300) {
        min_pump_run_sec_ = config->min_pump_run_sec;
    }
    
    if (config->min_pump_off_sec >= 60 && config->min_pump_off_sec <= 3600) {
        min_pump_off_sec_ = config->min_pump_off_sec;
    }
    
    if (config->max_pump_off_sec >= 300 && config->max_pump_off_sec <= 7200) {
        max_pump_off_sec_ = config->max_pump_off_sec;
    }
    
    fs.freeFile(data);
    printf("Config loaded from LittleFS\n");
    return true;
}
