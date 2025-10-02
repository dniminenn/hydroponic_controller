#include "pump_controller.h"
#include "../sensors/sensor_manager.h"
#include "../utils/gpio_utils.h"
#include "pico/stdlib.h"
#include <stdio.h>

PumpController::PumpController(SensorManager* sensor_manager) 
    : sensor_manager_(sensor_manager) {
    ConfigManager& config = ConfigManager::getInstance();
    on_time_ = config.getPumpOnSec();
    period_ = config.getPumpPeriod();
    humidity_mode_ = config.getHumidityMode();
    humidity_threshold_ = config.getHumidityThreshold();
    min_run_sec_ = config.getMinPumpRunSec();
    min_off_sec_ = config.getMinPumpOffSec();
    max_off_sec_ = config.getMaxPumpOffSec();
}

void PumpController::update() {
    if (humidity_mode_) {
        updateHumidityMode();
    } else {
        updateTimerMode();
    }
}

void PumpController::updateTimerMode() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time()) / 1000;
    
    if (!state_.is_on) {
        if (state_.next_start_time == 0) {
            state_.next_start_time = current_time;
        }
        
        if (current_time >= state_.next_start_time) {
            GpioUtils::setRelay(PIN_PUMP, true);
            state_.is_on = true;
            state_.on_start_time = current_time;
            printf("Pump ON (timer mode)\n");
        }
    } else {
        if (current_time - state_.on_start_time >= on_time_) {
            GpioUtils::setRelay(PIN_PUMP, false);
            state_.is_on = false;
            state_.next_start_time = current_time + (period_ - on_time_);
            printf("Pump OFF (timer mode) - next start in %u seconds\n", period_ - on_time_);
        }
    }
}

void PumpController::updateHumidityMode() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time()) / 1000;
    
    if (!sensor_manager_->isHumidityValid()) {
        // No humidity data - fall back to timer mode
        if (!state_.is_on) {
            if (state_.next_start_time == 0) {
                state_.next_start_time = current_time;
            }
            
            if (current_time >= state_.next_start_time) {
                GpioUtils::setRelay(PIN_PUMP, true);
                state_.is_on = true;
                state_.on_start_time = current_time;
                printf("Pump ON (timer fallback - no humidity data)\n");
            }
        } else {
            if (current_time - state_.on_start_time >= on_time_) {
                GpioUtils::setRelay(PIN_PUMP, false);
                state_.is_on = false;
                state_.next_start_time = current_time + (period_ - on_time_);
                printf("Pump OFF (timer fallback) - next start in %u seconds\n", period_ - on_time_);
            }
        }
        return;
    }
    
    float humidity = sensor_manager_->getLastHumidity();
    
    // Humidity control logic with timing constraints
    bool minRunTimeElapsed = !state_.is_on || (current_time - state_.on_start_time >= min_run_sec_);
    bool minOffTimeElapsed = !state_.is_on && (state_.next_start_time == 0 || current_time >= state_.next_start_time);
    bool maxOffTimeExceeded = !state_.is_on && state_.next_start_time > 0 && 
                             (current_time - (state_.next_start_time - min_off_sec_) >= max_off_sec_);
    
    // Turn ON when: humidity LOW and constraints met, or max off time exceeded
    // Turn OFF when: humidity HIGH and min run time met
    bool should_be_on = state_.is_on ? 
        !(humidity >= humidity_threshold_ && minRunTimeElapsed) :  // Stay on unless humidity high AND min run time met
        ((humidity < humidity_threshold_ && minOffTimeElapsed) || maxOffTimeExceeded);
    
    if (should_be_on && !state_.is_on) {
        GpioUtils::setRelay(PIN_PUMP, true);
        state_.is_on = true;
        state_.on_start_time = current_time;
        if (maxOffTimeExceeded) {
            printf("Pump ON (SAFETY - max off time exceeded) - %.1f%%\n", humidity);
        } else {
            printf("Pump ON (humidity control) - %.1f%% < %.1f%% (threshold)\n", humidity, humidity_threshold_);
        }
    } else if (!should_be_on && state_.is_on && minRunTimeElapsed) {
        GpioUtils::setRelay(PIN_PUMP, false);
        state_.is_on = false;
        state_.next_start_time = current_time + min_off_sec_;
        printf("Pump OFF (humidity control) - %.1f%% >= %.1f%% (threshold), next start: %ds\n", 
               humidity, humidity_threshold_, min_off_sec_);
    }
}

void PumpController::reset() {
    state_.is_on = false;
    state_.next_start_time = 0;
    state_.on_start_time = 0;
    GpioUtils::setRelay(PIN_PUMP, false);
}

void PumpController::setTiming(uint32_t on_sec, uint32_t period_sec) {
    on_time_ = on_sec;
    period_ = period_sec;
    
    ConfigManager& config = ConfigManager::getInstance();
    config.setPumpOnSec(on_sec);
    config.setPumpPeriod(period_sec);
}

void PumpController::setHumidityMode(bool enabled) {
    humidity_mode_ = enabled;
    ConfigManager& config = ConfigManager::getInstance();
    config.setHumidityMode(enabled);
    
    // Reset pump state when switching modes
    reset();
}

void PumpController::setHumidityThreshold(float threshold) {
    humidity_threshold_ = threshold;
    ConfigManager& config = ConfigManager::getInstance();
    config.setHumidityThreshold(threshold);
}

void PumpController::setMinRunTime(uint32_t seconds) {
    min_run_sec_ = seconds;
    ConfigManager& config = ConfigManager::getInstance();
    config.setMinPumpRunSec(seconds);
}

void PumpController::setMinOffTime(uint32_t seconds) {
    min_off_sec_ = seconds;
    ConfigManager& config = ConfigManager::getInstance();
    config.setMinPumpOffSec(seconds);
}

void PumpController::setMaxOffTime(uint32_t seconds) {
    max_off_sec_ = seconds;
    ConfigManager& config = ConfigManager::getInstance();
    config.setMaxPumpOffSec(seconds);
}
