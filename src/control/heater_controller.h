#pragma once

#include "control_base.h"
#include "../config.h"

class SensorManager;

class HeaterController : public ControlBase {
public:
    HeaterController(SensorManager* sensor_manager);
    
    void update() override;
    void reset() override;
    bool isOn() const override { return state_.is_on; }
    const char* getName() const override { return "Heater"; }
    
    // Configuration access
    void setSetpoint(float setpoint_c);
    float getSetpoint() const { return setpoint_c_; }
    
private:
    SensorManager* sensor_manager_;
    ChannelState state_;
    float setpoint_c_;
    static constexpr float HYSTERESIS_C = 0.5f;
};
