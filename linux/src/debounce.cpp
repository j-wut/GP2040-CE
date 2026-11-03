/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#include "debounce.h"

namespace gp2040linux {

uint64_t Debouncer::update(uint64_t raw, uint64_t nowNs) {
    // Release lines whose lock-out has ended.
    uint64_t l = locked;
    while (l) {
        int i = __builtin_ctzll(l);
        l &= l - 1;
        if (nowNs >= unlockAt[i]) {
            locked &= ~(1ull << i);
        }
    }

    uint64_t changed = (raw ^ stable) & ~locked;
    if (period == 0) {
        stable ^= changed;
        return stable;
    }
    while (changed) {
        int i = __builtin_ctzll(changed);
        changed &= changed - 1;
        stable ^= 1ull << i;
        locked |= 1ull << i;
        unlockAt[i] = nowNs + period;
    }
    return stable;
}

uint64_t Debouncer::nextDeadline() const {
    uint64_t best = 0;
    uint64_t l = locked;
    while (l) {
        int i = __builtin_ctzll(l);
        l &= l - 1;
        if (best == 0 || unlockAt[i] < best) {
            best = unlockAt[i];
        }
    }
    return best;
}

} // namespace gp2040linux
