/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstdint>

namespace gp2040linux {

// Eager ("lock-out") debounce over up to 64 lines.
//
// A change on a line is accepted at once, so a press adds no latency. The line
// is then locked for `periodNs`, and bounces inside that window are ignored.
// When the window ends the caller samples again (see nextDeadline()) so a
// release that happened during the lock-out is not lost.
class Debouncer {
public:
    explicit Debouncer(uint64_t periodNs = 0) : period(periodNs) {}

    void setPeriod(uint64_t periodNs) { period = periodNs; }

    // Feed a raw sample (bit set = active). Returns the debounced mask.
    uint64_t update(uint64_t raw, uint64_t nowNs);

    uint64_t state() const { return stable; }

    // Earliest time a locked line unlocks, or 0 when nothing is locked.
    uint64_t nextDeadline() const;

private:
    uint64_t period;
    uint64_t stable = 0;
    uint64_t locked = 0;
    uint64_t unlockAt[64] = {};
};

} // namespace gp2040linux
