#include "nano_nrf_receiver.h"
#include <string.h>

NanoNRFReceiver::NanoNRFReceiver(NRF24L01* nrf, uint8_t pipe)
    : nrf_(nrf), pipe_(pipe) {
    memset(values_, 0, sizeof(values_));
}

bool NanoNRFReceiver::read() {
    if (!nrf_->available()) {
        return false;
    }
    
    uint8_t buffer[16];
    if (nrf_->read(buffer, sizeof(buffer))) {
        memcpy(values_, buffer, sizeof(values_));
        return true;
    }
    
    return false;
}

float NanoNRFReceiver::getValue(int index) {
    if (index >= 0 && index < 4) {
        return values_[index];
    }
    return -999.0f;
}

