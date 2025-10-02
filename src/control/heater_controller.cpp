#include "heater_controller.h"
#include "../sensors/sensor_manager.h"
#include "../utils/gpio_utils.h"
#include "pico/stdlib.h"
#include <stdio.h>

HeaterController::HeaterController(SensorManager* sensor_manager) 
    : sensor_manager_(sensor_manager) {
    ConfigManager& config = ConfigManager::getInstance();
    setpoint_c_ = config.getHeaterSetpointC();
}

void HeaterController::update() {
    if (!sensor_manager_->isTemperatureValid()) return;  // No temperature reading available
    
    float temperature = sensor_manager_->getLastTemperature();
    float onThreshold = setpoint_c_ - HYSTERESIS_C;
    float offThreshold = setpoint_c_ + HYSTERESIS_C;
    bool should_be_on = state_.is_on ? (temperature < offThreshold) : (temperature < onThreshold);
    
    if (should_be_on && !state_.is_on) {
        GpioUtils::setRelay(PIN_HEATER, true);
        state_.is_on = true;
        state_.on_start_time = to_ms_since_boot(get_absolute_time()) / 1000;
        printf("Heater ON\n");
    } else if (!should_be_on && state_.is_on) {
        GpioUtils::setRelay(PIN_HEATER, false);
        state_.is_on = false;
        printf("Heater OFF\n");
    }
}

void HeaterController::reset() {
    state_.is_on = false;
    state_.next_start_time = 0;
    state_.on_start_time = 0;
    GpioUtils::setRelay(PIN_HEATER, false);
}

void HeaterController::setSetpoint(float setpoint_c) {
    setpoint_c_ = setpoint_c;
    ConfigManager& config = ConfigManager::getInstance();
    config.setHeaterSetpointC(setpoint_c);
}
