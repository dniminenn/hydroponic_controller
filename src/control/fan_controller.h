#pragma once

#include "control_base.h"
#include "../config.h"

class SensorManager;

class FanController : public ControlBase {
public:
    FanController(SensorManager* sensor_manager);
    
    void update() override;
    void reset() override;
    bool isOn() const override { return fan_on_; }
    const char* getName() const override { return "Fan"; }
    
    // Manual control
    void setManualControl(bool on);
    bool isManualControl() const { return fan_manual_control_; }
    
    // Temperature thresholds
    static constexpr float FAN_ON_TEMP_C = 24.0f;
    static constexpr float FAN_OFF_TEMP_C = 15.0f;
    
private:
    SensorManager* sensor_manager_;
    bool fan_on_;
    bool fan_manual_control_;
};
