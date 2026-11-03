/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#include "report_sender.h"

#include <cerrno>
#include <cstring>

namespace gp2040linux {

bool ReportSender::submit(uint8_t const *report, size_t len, uint64_t nowNs) {
    if (len == 0 || len > MAX_REPORT_LENGTH) {
        return false;
    }
    bool const sameAsSent = len == lastSentLen && std::memcmp(report, lastSent, len) == 0;
    if (sameAsSent && !pending) {
        return false; // nothing new for the host
    }
    if (pending) {
        replaced++; // drop the stale held report, keep only the newest
    }
    std::memcpy(latest, report, len);
    latestLen = len;
    if (sameAsSent) {
        // Input went back to what the host already has; nothing to send.
        pending = false;
        return false;
    }
    pending = true;
    return tryWrite(nowNs);
}

bool ReportSender::flush(uint64_t nowNs) {
    if (!pending) {
        return false;
    }
    return tryWrite(nowNs);
}

bool ReportSender::tick(uint64_t nowNs) {
    if (pending) {
        return tryWrite(nowNs);
    }
    if (heartbeatNs == 0 || latestLen == 0 || nowNs - lastWriteNs < heartbeatNs) {
        return false;
    }
    pending = true;
    return tryWrite(nowNs);
}

uint64_t ReportSender::nextHeartbeat() const {
    if (heartbeatNs == 0 || latestLen == 0) {
        return 0;
    }
    return lastWriteNs + heartbeatNs;
}

bool ReportSender::tryWrite(uint64_t nowNs) {
    long n = writeFn(latest, latestLen);
    if (n == static_cast<long>(latestLen)) {
        std::memcpy(lastSent, latest, latestLen);
        lastSentLen = latestLen;
        pending = false;
        lastWriteNs = nowNs;
        sent++;
        return true;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        busy++;
    } else {
        // Host gone, suspended, or a short write. Keep the report; the caller
        // retries when the device is writable again.
        errors++;
    }
    return false;
}

} // namespace gp2040linux
