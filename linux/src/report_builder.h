/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "input_state.h"

namespace gp2040linux {

// Input modes the Linux port can present over the kernel's f_hid function.
// They reuse the firmware's own descriptors from headers/drivers/.
enum class OutputMode {
    Switch,  // HORI/Pokken-style Switch HID (headers/drivers/switch)
    DInput,  // GP2040-CE generic HID / DirectInput (headers/drivers/hid)
};

bool parseOutputMode(std::string const &name, OutputMode &mode);
char const *outputModeName(OutputMode mode);

struct GadgetIdentity {
    uint16_t vid;
    uint16_t pid;
    uint16_t bcdDevice;
    char const *manufacturer;
    char const *product;
    uint8_t const *reportDescriptor;
    size_t reportDescriptorLength;
    size_t reportLength;
};

GadgetIdentity gadgetIdentity(OutputMode mode);

constexpr size_t MAX_REPORT_LENGTH = 64;

// Build the full-state input report for `mode` into `out`. Returns its length.
size_t buildReport(OutputMode mode, GamepadState const &state, uint8_t *out);

} // namespace gp2040linux
