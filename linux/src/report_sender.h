/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "report_builder.h"

namespace gp2040linux {

// Latest-only report sender for an f_hid device (/dev/hidgN).
//
// f_hid holds one IN transfer in flight. While it is in flight, a non-blocking
// write() fails with EAGAIN. We never queue behind it: we keep only the newest
// full-state report here and write it as soon as the device is writable, so the
// host always gets current state and never a backlog of stale reports.
//
// Reports are only written when they change (send on change). An optional
// heartbeat re-sends the current report at a fixed rate.
class ReportSender {
public:
    // Write hook returns bytes written, or -1 with errno set (EAGAIN = busy).
    using WriteFn = std::function<long(uint8_t const *, size_t)>;

    explicit ReportSender(WriteFn write) : writeFn(std::move(write)) {}

    void setHeartbeatNs(uint64_t ns) { heartbeatNs = ns; }

    // Offer the newest report. Returns true if it went out now.
    bool submit(uint8_t const *report, size_t len, uint64_t nowNs);

    // Retry a report that was held back because the device was busy.
    bool flush(uint64_t nowNs);

    // Heartbeat check; call from the timer.
    bool tick(uint64_t nowNs);

    bool hasPending() const { return pending; }
    uint64_t nextHeartbeat() const;

    uint64_t sentCount() const { return sent; }
    uint64_t busyCount() const { return busy; }
    uint64_t replacedCount() const { return replaced; }
    uint64_t errorCount() const { return errors; }

private:
    bool tryWrite(uint64_t nowNs);

    WriteFn writeFn;
    uint8_t latest[MAX_REPORT_LENGTH] = {};
    size_t latestLen = 0;
    uint8_t lastSent[MAX_REPORT_LENGTH] = {};
    size_t lastSentLen = 0;
    bool pending = false;
    uint64_t heartbeatNs = 0;
    uint64_t lastWriteNs = 0;
    uint64_t sent = 0;
    uint64_t busy = 0;
    uint64_t replaced = 0;
    uint64_t errors = 0;
};

} // namespace gp2040linux
