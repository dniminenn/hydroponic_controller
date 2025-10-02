#pragma once

#include "control_base.h"
#include "../config.h"

class SensorManager;

class PumpController : public ControlBase {
public:
    PumpController(SensorManager* sensor_manager);
    
    void update() override;
    void reset() override;
    bool isOn() const override { return state_.is_on; }
    const char* getName() const override { return "Pump"; }
    
    // Configuration access
    void setTiming(uint32_t on_sec, uint32_t period_sec);
    void setHumidityMode(bool enabled);
    void setHumidityThreshold(float threshold);
    void setMinRunTime(uint32_t seconds);
    void setMinOffTime(uint32_t seconds);
    void setMaxOffTime(uint32_t seconds);
    
    uint32_t getOnTime() const { return on_time_; }
    uint32_t getPeriod() const { return period_; }
    bool isHumidityMode() const { return humidity_mode_; }
    float getHumidityThreshold() const { return humidity_threshold_; }
    
private:
    void updateTimerMode();
    void updateHumidityMode();
    
    SensorManager* sensor_manager_;
    ChannelState state_;
    
    // Configuration
    uint32_t on_time_;
    uint32_t period_;
    bool humidity_mode_;
    float humidity_threshold_;
    uint32_t min_run_sec_;
    uint32_t min_off_sec_;
    uint32_t max_off_sec_;
};
