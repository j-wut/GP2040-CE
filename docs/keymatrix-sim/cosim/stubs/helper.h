#pragma once
#include <stdint.h>
#include "pico.h"
static inline bool isValidPin(int32_t pin) { return pin >= 0 && pin < NUM_BANK0_GPIOS; }
