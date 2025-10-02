#pragma once

#include "control_base.h"
#include "../config.h"
#include "../utils/time_utils.h"

class LightsController : public ControlBase {
public:
    LightsController();
    
    void update() override;
    void reset() override;
    bool isOn() const override { return state_.is_on; }
    const char* getName() const override { return "Lights"; }
    
    // Configuration access
    void setSchedule(uint32_t start_s, uint32_t end_s);
    uint32_t getStartTime() const { return start_time_; }
    uint32_t getEndTime() const { return end_time_; }
    
private:
    ChannelState state_;
    uint32_t start_time_;
    uint32_t end_time_;
};
