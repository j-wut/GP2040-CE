// Mock GPIO: the co-simulation harness implements these (harness.cpp)
#pragma once
#include <stdint.h>
#define GPIO_IN false
#define GPIO_OUT true
void gpio_init(unsigned pin);
void gpio_set_dir(unsigned pin, bool out);
void gpio_pull_up(unsigned pin);
void gpio_put(unsigned pin, bool value);
uint64_t gpio_get_all64();
