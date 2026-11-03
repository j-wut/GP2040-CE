/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstdint>

// Button bits match GAMEPAD_MASK_* in headers/gamepad/GamepadState.h so the
// report builders read the same way as the firmware drivers.
namespace gp2040linux {

enum Button : uint32_t {
    BTN_B1 = 1u << 0,
    BTN_B2 = 1u << 1,
    BTN_B3 = 1u << 2,
    BTN_B4 = 1u << 3,
    BTN_L1 = 1u << 4,
    BTN_R1 = 1u << 5,
    BTN_L2 = 1u << 6,
    BTN_R2 = 1u << 7,
    BTN_S1 = 1u << 8,
    BTN_S2 = 1u << 9,
    BTN_L3 = 1u << 10,
    BTN_R3 = 1u << 11,
    BTN_A1 = 1u << 12,
    BTN_A2 = 1u << 13,
    BTN_A3 = 1u << 14,
    BTN_A4 = 1u << 15,
};

enum Dpad : uint8_t {
    DPAD_UP = 1u << 0,
    DPAD_DOWN = 1u << 1,
    DPAD_LEFT = 1u << 2,
    DPAD_RIGHT = 1u << 3,
    DPAD_MASK = 0x0F,
};

constexpr uint16_t AXIS_MIN = 0;
constexpr uint16_t AXIS_MID = 0x8000;
constexpr uint16_t AXIS_MAX = 0xFFFF;

// Full gamepad state. Every report is built from one of these, never from deltas.
struct GamepadState {
    uint32_t buttons = 0;
    uint8_t dpad = 0;
    uint16_t lx = AXIS_MID;
    uint16_t ly = AXIS_MID;
    uint16_t rx = AXIS_MID;
    uint16_t ry = AXIS_MID;
    uint8_t lt = 0;
    uint8_t rt = 0;

    bool operator==(GamepadState const &o) const {
        return buttons == o.buttons && dpad == o.dpad && lx == o.lx && ly == o.ly &&
               rx == o.rx && ry == o.ry && lt == o.lt && rt == o.rt;
    }
    bool operator!=(GamepadState const &o) const { return !(*this == o); }
};

} // namespace gp2040linux
