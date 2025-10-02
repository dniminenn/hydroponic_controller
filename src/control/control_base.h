#pragma once

#include <stdint.h>
#include <stdbool.h>

// State structure for control channels
struct ChannelState {
    bool is_on = false;
    uint32_t next_start_time = 0;
    uint32_t on_start_time = 0;
};

// Base class for all control systems
class ControlBase {
public:
    virtual ~ControlBase() = default;
    virtual void update() = 0;
    virtual void reset() = 0;
    virtual bool isOn() const = 0;
    virtual const char* getName() const = 0;
};
