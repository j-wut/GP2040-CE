/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#include "report_builder.h"

#include <cstring>

#include "drivers/hid/HIDDescriptors.h"
#include "drivers/switch/SwitchDescriptors.h"

namespace gp2040linux {

static_assert(sizeof(SwitchReport) <= MAX_REPORT_LENGTH, "Switch report too large");
static_assert(sizeof(HIDReport) <= MAX_REPORT_LENGTH, "HID report too large");

bool parseOutputMode(std::string const &name, OutputMode &mode) {
    if (name == "switch") {
        mode = OutputMode::Switch;
        return true;
    }
    if (name == "dinput" || name == "hid") {
        mode = OutputMode::DInput;
        return true;
    }
    return false;
}

char const *outputModeName(OutputMode mode) {
    switch (mode) {
        case OutputMode::Switch: return "switch";
        case OutputMode::DInput: return "dinput";
    }
    return "unknown";
}

static uint16_t descriptorWord(uint8_t const *desc, size_t offset) {
    return static_cast<uint16_t>(desc[offset] | (desc[offset + 1] << 8));
}

GadgetIdentity gadgetIdentity(OutputMode mode) {
    switch (mode) {
        case OutputMode::Switch:
            return {
                descriptorWord(switch_device_descriptor, 8),
                descriptorWord(switch_device_descriptor, 10),
                descriptorWord(switch_device_descriptor, 12),
                reinterpret_cast<char const *>(switch_string_manufacturer),
                reinterpret_cast<char const *>(switch_string_product),
                switch_report_descriptor,
                sizeof(switch_report_descriptor),
                sizeof(SwitchReport),
            };
        case OutputMode::DInput:
        default:
            return {
                descriptorWord(hid_device_descriptor, 8),
                descriptorWord(hid_device_descriptor, 10),
                descriptorWord(hid_device_descriptor, 12),
                reinterpret_cast<char const *>(hid_string_manufacturer),
                reinterpret_cast<char const *>(hid_string_product),
                hid_report_descriptor,
                sizeof(hid_report_descriptor),
                sizeof(HIDReport),
            };
    }
}

template <typename T>
static uint8_t hatFromDpad(uint8_t dpad, T up, T upRight, T right, T downRight, T down,
                           T downLeft, T left, T upLeft, T none) {
    switch (dpad & DPAD_MASK) {
        case DPAD_UP:                return up;
        case DPAD_UP | DPAD_RIGHT:   return upRight;
        case DPAD_RIGHT:             return right;
        case DPAD_DOWN | DPAD_RIGHT: return downRight;
        case DPAD_DOWN:              return down;
        case DPAD_DOWN | DPAD_LEFT:  return downLeft;
        case DPAD_LEFT:              return left;
        case DPAD_UP | DPAD_LEFT:    return upLeft;
        default:                     return none;
    }
}

static size_t buildSwitch(GamepadState const &s, uint8_t *out) {
    // Mapping mirrors src/drivers/switch/SwitchDriver.cpp.
    SwitchReport r{};
    r.hat = hatFromDpad<uint8_t>(s.dpad, SWITCH_HAT_UP, SWITCH_HAT_UPRIGHT, SWITCH_HAT_RIGHT,
                                 SWITCH_HAT_DOWNRIGHT, SWITCH_HAT_DOWN, SWITCH_HAT_DOWNLEFT,
                                 SWITCH_HAT_LEFT, SWITCH_HAT_UPLEFT, SWITCH_HAT_NOTHING);
    uint32_t const b = s.buttons;
    r.buttons = static_cast<uint16_t>(0
        | ((b & BTN_B1) ? SWITCH_MASK_B       : 0)
        | ((b & BTN_B2) ? SWITCH_MASK_A       : 0)
        | ((b & BTN_B3) ? SWITCH_MASK_Y       : 0)
        | ((b & BTN_B4) ? SWITCH_MASK_X       : 0)
        | ((b & BTN_L1) ? SWITCH_MASK_L       : 0)
        | ((b & BTN_R1) ? SWITCH_MASK_R       : 0)
        | ((b & BTN_L2) ? SWITCH_MASK_ZL      : 0)
        | ((b & BTN_R2) ? SWITCH_MASK_ZR      : 0)
        | ((b & BTN_S1) ? SWITCH_MASK_MINUS   : 0)
        | ((b & BTN_S2) ? SWITCH_MASK_PLUS    : 0)
        | ((b & BTN_L3) ? SWITCH_MASK_L3      : 0)
        | ((b & BTN_R3) ? SWITCH_MASK_R3      : 0)
        | ((b & BTN_A1) ? SWITCH_MASK_HOME    : 0)
        | ((b & BTN_A2) ? SWITCH_MASK_CAPTURE : 0));
    if (s.lt > 0) r.buttons |= SWITCH_MASK_ZL;
    if (s.rt > 0) r.buttons |= SWITCH_MASK_ZR;
    r.lx = static_cast<uint8_t>(s.lx >> 8);
    r.ly = static_cast<uint8_t>(s.ly >> 8);
    r.rx = static_cast<uint8_t>(s.rx >> 8);
    r.ry = static_cast<uint8_t>(s.ry >> 8);
    r.vendor = 0;
    std::memcpy(out, &r, sizeof(r));
    return sizeof(r);
}

static size_t buildDInput(GamepadState const &s, uint8_t *out) {
    // Mapping mirrors src/drivers/hid/HIDDriver.cpp. That driver swaps
    // B1/B2/B3 to match the report descriptor's button order.
    HIDReport r{};
    r.direction = hatFromDpad<uint8_t>(s.dpad, HID_HAT_UP, HID_HAT_UPRIGHT, HID_HAT_RIGHT,
                                       HID_HAT_DOWNRIGHT, HID_HAT_DOWN, HID_HAT_DOWNLEFT,
                                       HID_HAT_LEFT, HID_HAT_UPLEFT, HID_HAT_NOTHING);
    r.dummy = 0;
    uint32_t const b = s.buttons;
    r.buttons = 0
        | ((b & BTN_B1) ? uint32_t(BTN_B2) : 0u)
        | ((b & BTN_B2) ? uint32_t(BTN_B3) : 0u)
        | ((b & BTN_B3) ? uint32_t(BTN_B1) : 0u)
        | (b & (BTN_B4 | BTN_L1 | BTN_R1 | BTN_L2 | BTN_R2 | BTN_S1 | BTN_S2 | BTN_L3 |
                BTN_R3 | BTN_A1 | BTN_A2 | BTN_A3 | BTN_A4));
    // The firmware also sets d-pad bits 16..19 alongside the hat.
    if (s.dpad & DPAD_UP) r.buttons |= 1u << 16;
    if (s.dpad & DPAD_DOWN) r.buttons |= 1u << 17;
    if (s.dpad & DPAD_LEFT) r.buttons |= 1u << 18;
    if (s.dpad & DPAD_RIGHT) r.buttons |= 1u << 19;
    r.l_x_axis = static_cast<uint8_t>(s.lx >> 8);
    r.l_y_axis = static_cast<uint8_t>(s.ly >> 8);
    r.r_x_axis = static_cast<uint8_t>(s.rx >> 8);
    r.r_y_axis = static_cast<uint8_t>(s.ry >> 8);
    std::memcpy(out, &r, sizeof(r));
    return sizeof(r);
}

size_t buildReport(OutputMode mode, GamepadState const &state, uint8_t *out) {
    switch (mode) {
        case OutputMode::Switch: return buildSwitch(state, out);
        case OutputMode::DInput: return buildDInput(state, out);
    }
    return 0;
}

} // namespace gp2040linux
