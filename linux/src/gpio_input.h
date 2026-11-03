/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config.h"

namespace gp2040linux {

// GPIO inputs through the kernel GPIO character device (uAPI v2, Linux 5.10+).
// No libgpiod dependency, so the same binary runs on Pi OS Bookworm and later.
class GpioInput {
public:
    ~GpioInput();

    // Open the chip and request every mapped line as an input with both-edge
    // events. `chip` is a /dev path or "auto" (finds the 40-pin header chip).
    bool open(std::string const &chip, std::vector<PinMapping> const &pins, bool activeLow,
              Bias bias, bool edgeEvents, std::string &error);

    // File descriptor that becomes readable when an edge event is queued.
    int eventFd() const { return lineFd; }

    // Discard queued edge events. The caller then samples(), so no edge is
    // trusted over the current line level.
    void drainEvents();

    // Current active mask: bit i is set when pins[i] is active (pressed).
    bool sample(uint64_t &mask);

    std::string const &chipPath() const { return path; }

private:
    int lineFd = -1;
    unsigned count = 0;
    std::string path;
};

// Pick the chip that drives the 40-pin header: pinctrl-rp1 on Pi 5 / CM5,
// pinctrl-bcm2711 on Pi 4 / CM4, pinctrl-bcm2835 on older boards.
std::string findHeaderGpioChip();

} // namespace gp2040linux
