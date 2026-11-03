/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstdint>

#include "input_state.h"

namespace gp2040linux {

// Same choices as the firmware's SOCDMode.
enum class SocdMode {
    UpPriority,     // U+D = U, L+R = neutral (Hitbox)
    Neutral,        // U+D = neutral, L+R = neutral
    LastWins,       // second input priority
    FirstWins,      // first input priority
    Bypass,         // no cleaning
};

enum class DpadMode {
    Digital,
    LeftAnalog,
    RightAnalog,
};

// SOCD cleaning keeps a little history for the last/first-wins modes.
class SocdCleaner {
public:
    explicit SocdCleaner(SocdMode m = SocdMode::Neutral) : mode(m) {}
    void setMode(SocdMode m) { mode = m; }
    uint8_t clean(uint8_t dpad);

private:
    uint8_t cleanAxis(uint8_t dpad, uint8_t neg, uint8_t pos, uint8_t &lastPressed,
                      bool upPriorityAxis);
    SocdMode mode;
    uint8_t lastVertical = 0;
    uint8_t lastHorizontal = 0;
};

// Move the d-pad onto a stick when DpadMode asks for it.
void applyDpadMode(GamepadState &state, DpadMode mode);

// Deadzone plus hysteresis for 16-bit analog axes, so ADC noise does not
// turn into a stream of reports. Values inside the deadzone snap to center;
// outside it a new value is only taken when it moves more than `hysteresis`.
class AnalogFilter {
public:
    AnalogFilter(uint16_t deadzone = 0, uint16_t hysteresis = 0)
        : deadzone(deadzone), hysteresis(hysteresis) {}
    uint16_t filter(uint16_t raw);

private:
    uint16_t deadzone;
    uint16_t hysteresis;
    uint16_t last = AXIS_MID;
};

} // namespace gp2040linux
