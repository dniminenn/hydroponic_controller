#include "lights_controller.h"
#include "../utils/gpio_utils.h"
#include "pico/stdlib.h"
#include <stdio.h>

LightsController::LightsController() {
    ConfigManager& config = ConfigManager::getInstance();
    start_time_ = config.getLightsStartS();
    end_time_ = config.getLightsEndS();
}

void LightsController::update() {
    uint32_t current_seconds = TimeUtils::getSecondsFromMidnight();
    if (current_seconds == 0) return;  // No time available
    
    bool should_be_on = (start_time_ <= end_time_)
        ? (current_seconds >= start_time_ && current_seconds < end_time_)
        : (current_seconds >= start_time_ || current_seconds < end_time_);
    
    if (should_be_on && !state_.is_on) {
        GpioUtils::setRelay(PIN_LIGHTS, true);
        state_.is_on = true;
        state_.on_start_time = to_ms_since_boot(get_absolute_time()) / 1000;
        printf("Lights ON\n");
    } else if (!should_be_on && state_.is_on) {
        GpioUtils::setRelay(PIN_LIGHTS, false);
        state_.is_on = false;
        printf("Lights OFF\n");
    }
}

void LightsController::reset() {
    state_.is_on = false;
    state_.next_start_time = 0;
    state_.on_start_time = 0;
    GpioUtils::setRelay(PIN_LIGHTS, false);
}

void LightsController::setSchedule(uint32_t start_s, uint32_t end_s) {
    start_time_ = start_s;
    end_time_ = end_s;
    
    // Update configuration
    ConfigManager& config = ConfigManager::getInstance();
    config.setLightsStartS(start_s);
    config.setLightsEndS(end_s);
}
