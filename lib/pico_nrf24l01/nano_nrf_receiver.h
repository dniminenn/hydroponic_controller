#pragma once

#include <stdint.h>
#include "nrf24l01.h"

// Wrapper for receiving Nano ADC data via NRF24L01
class NanoNRFReceiver {
public:
    NanoNRFReceiver(NRF24L01* nrf, uint8_t pipe);
    
    bool read();
    float getValue(int index);
    const float* getValues() { return values_; }
    
private:
    NRF24L01* nrf_;
    uint8_t pipe_;
    float values_[4];
};

