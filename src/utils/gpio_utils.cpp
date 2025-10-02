#include "gpio_utils.h"
#include "hardware/gpio.h"
#include <stdio.h>

void GpioUtils::initializeGpioOutputs() {
    const uint8_t pins[] = { PIN_LIGHTS, PIN_PUMP, PIN_HEATER, PIN_FAN };
    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t pin = pins[i];
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);
    }
}

void GpioUtils::setRelay(uint8_t pin, bool on) {
#if ACTIVE_HIGH
    gpio_put(pin, on ? 1 : 0);
#else
    gpio_put(pin, on ? 0 : 1);
#endif
}

void GpioUtils::setAllRelaysOff() {
    setRelay(PIN_LIGHTS, false);
    setRelay(PIN_PUMP, false);
    setRelay(PIN_HEATER, false);
    setRelay(PIN_FAN, false);
}
