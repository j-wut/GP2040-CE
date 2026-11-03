/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#include "gamepad_logic.h"

#include <cstdlib>

namespace gp2040linux {

uint8_t SocdCleaner::cleanAxis(uint8_t dpad, uint8_t neg, uint8_t pos, uint8_t &lastPressed,
                               bool upPriorityAxis) {
    uint8_t const both = neg | pos;
    uint8_t const axis = dpad & both;
    if (axis != both) {
        // Remember the single direction held before any conflict.
        lastPressed = axis;
        return axis;
    }

    switch (mode) {
        case SocdMode::Bypass:
            return both;
        case SocdMode::UpPriority:
            return upPriorityAxis ? neg : 0; // neg is UP on the vertical axis
        case SocdMode::Neutral:
            return 0;
        case SocdMode::LastWins:
            return lastPressed ? (both & ~lastPressed) : 0;
        case SocdMode::FirstWins:
            return lastPressed;
    }
    return 0;
}

uint8_t SocdCleaner::clean(uint8_t dpad) {
    dpad &= DPAD_MASK;
    uint8_t out = 0;
    out |= cleanAxis(dpad, DPAD_UP, DPAD_DOWN, lastVertical, true);
    out |= cleanAxis(dpad, DPAD_LEFT, DPAD_RIGHT, lastHorizontal, false);
    return out;
}

void applyDpadMode(GamepadState &state, DpadMode mode) {
    if (mode == DpadMode::Digital) {
        return;
    }
    uint16_t x = AXIS_MID;
    uint16_t y = AXIS_MID;
    if (state.dpad & DPAD_LEFT) x = AXIS_MIN;
    if (state.dpad & DPAD_RIGHT) x = AXIS_MAX;
    if (state.dpad & DPAD_UP) y = AXIS_MIN;
    if (state.dpad & DPAD_DOWN) y = AXIS_MAX;
    if (mode == DpadMode::LeftAnalog) {
        state.lx = x;
        state.ly = y;
    } else {
        state.rx = x;
        state.ry = y;
    }
    state.dpad = 0;
}

uint16_t AnalogFilter::filter(uint16_t raw) {
    int offset = static_cast<int>(raw) - AXIS_MID;
    if (std::abs(offset) <= deadzone) {
        last = AXIS_MID;
        return last;
    }
    if (std::abs(static_cast<int>(raw) - static_cast<int>(last)) > hysteresis ||
        raw == AXIS_MIN || raw == AXIS_MAX) {
        last = raw;
    }
    return last;
}

} // namespace gp2040linux
