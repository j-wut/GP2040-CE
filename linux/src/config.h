/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "gamepad_logic.h"
#include "report_builder.h"

namespace gp2040linux {

enum class InputMethod {
    Events,    // GPIO edge interrupts, thread sleeps between changes
    BusyPoll,  // tight sampling loop, best pinned to an isolated core
};

enum class Bias { PullUp, PullDown, None };

// One GPIO line and what it drives. Exactly one of button/dpad is non-zero.
struct PinMapping {
    unsigned gpio;
    uint32_t button;
    uint8_t dpad;
};

struct Config {
    OutputMode mode = OutputMode::Switch;
    // Requested host polling rate. On a High Speed link the interrupt endpoint
    // bInterval is set from this (8000 Hz -> bInterval 1 = every 125 us
    // microframe). The host still decides whether to honour it.
    unsigned pollRateHz = 8000;
    unsigned heartbeatHz = 0;        // 0 = send on change only
    unsigned debounceUs = 0;
    SocdMode socd = SocdMode::Neutral;
    DpadMode dpadMode = DpadMode::Digital;
    InputMethod inputMethod = InputMethod::Events;
    unsigned busyPollIntervalUs = 0; // 0 = spin
    int cpu = -1;                    // pin the input thread to this CPU
    int rtPriority = 0;              // SCHED_FIFO priority, 0 = normal scheduling
    std::string gpioChip = "auto";
    std::string hidgDevice = "/dev/hidg0";
    bool activeLow = true;
    Bias bias = Bias::PullUp;
    std::vector<PinMapping> pins;
};

// Parse `key = value` lines. Returns false and fills `error` on bad input.
bool parseConfig(std::string const &text, Config &cfg, std::string &error);
bool loadConfig(std::string const &path, Config &cfg, std::string &error);

// High Speed bInterval for a polling rate: 2^(bInterval-1) x 125 us.
// Returns 0 if the rate is not one of 8000/4000/2000/1000/500/250/125.
unsigned highSpeedIntervalForRate(unsigned hz);

} // namespace gp2040linux
