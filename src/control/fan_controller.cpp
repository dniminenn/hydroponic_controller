#include "fan_controller.h"
#include "../sensors/sensor_manager.h"
#include "../utils/gpio_utils.h"
#include "pico/stdlib.h"
#include <stdio.h>

FanController::FanController(SensorManager* sensor_manager) 
    : sensor_manager_(sensor_manager), fan_on_(false), fan_manual_control_(false) {
}

void FanController::update() {
    if (!sensor_manager_->isTemperatureValid()) return;  // No temperature reading available
    
    float temperature = sensor_manager_->getLastTemperature();
    
    // Above 24°C: uncontrollably ON
    if (temperature >= FAN_ON_TEMP_C) {
        if (!fan_on_) {
            GpioUtils::setRelay(PIN_FAN, true);
            fan_on_ = true;
            fan_manual_control_ = false;
            printf("Fan ON (temperature %.1f°C >= %.1f°C - uncontrollable)\n", temperature, FAN_ON_TEMP_C);
        }
    }
    // Below 15°C: uncontrollably OFF
    else if (temperature <= FAN_OFF_TEMP_C) {
        if (fan_on_) {
            GpioUtils::setRelay(PIN_FAN, false);
            fan_on_ = false;
            fan_manual_control_ = false;
            printf("Fan OFF (temperature %.1f°C <= %.1f°C - uncontrollable)\n", temperature, FAN_OFF_TEMP_C);
        }
    }
    // Between 15-24°C: manual control zone (no automatic changes)
}

void FanController::reset() {
    fan_on_ = false;
    fan_manual_control_ = false;
    GpioUtils::setRelay(PIN_FAN, false);
}

void FanController::setManualControl(bool on) {
    if (!sensor_manager_->isTemperatureValid()) return;
    
    float temperature = sensor_manager_->getLastTemperature();
    
    // Check if we're in manual control zone (15-24°C)
    if (temperature >= FAN_OFF_TEMP_C && temperature <= FAN_ON_TEMP_C) {
        GpioUtils::setRelay(PIN_FAN, on);
        fan_on_ = on;
        fan_manual_control_ = true;
        printf("Fan %s (manual control at %.1f°C)\n", on ? "ON" : "OFF", temperature);
    }
}
