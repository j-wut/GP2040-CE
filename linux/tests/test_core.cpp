/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

// Host-side tests for the Linux port's input and report logic.

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>

#include "config.h"
#include "debounce.h"
#include "drivers/hid/HIDDescriptors.h"
#include "drivers/switch/SwitchDescriptors.h"
#include "gamepad_logic.h"
#include "report_builder.h"
#include "report_sender.h"

using namespace gp2040linux;

static int failures = 0;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                             \
        }                                                                           \
    } while (0)

static void testDebounce() {
    Debouncer d(5000); // 5 us
    CHECK(d.update(0b1, 1000) == 0b1);      // press accepted at once
    CHECK(d.update(0b0, 2000) == 0b1);      // bounce ignored while locked
    CHECK(d.update(0b1, 3000) == 0b1);
    CHECK(d.nextDeadline() == 6000);
    CHECK(d.update(0b0, 6000) == 0b0);      // release taken after the window
    CHECK(d.update(0b10, 6500) == 0b10);    // other line independent, line 0 locked again
    Debouncer none(0);
    CHECK(none.update(0b1, 1) == 0b1);
    CHECK(none.update(0b0, 2) == 0b0);
    CHECK(none.nextDeadline() == 0);
}

static void testSocd() {
    SocdCleaner up(SocdMode::UpPriority);
    CHECK(up.clean(DPAD_UP | DPAD_DOWN) == DPAD_UP);
    CHECK(up.clean(DPAD_LEFT | DPAD_RIGHT) == 0);

    SocdCleaner neutral(SocdMode::Neutral);
    CHECK(neutral.clean(DPAD_UP | DPAD_DOWN | DPAD_LEFT) == DPAD_LEFT);

    SocdCleaner last(SocdMode::LastWins);
    CHECK(last.clean(DPAD_LEFT) == DPAD_LEFT);
    CHECK(last.clean(DPAD_LEFT | DPAD_RIGHT) == DPAD_RIGHT);
    CHECK(last.clean(DPAD_LEFT) == DPAD_LEFT);
    CHECK(last.clean(DPAD_RIGHT) == DPAD_RIGHT);
    CHECK(last.clean(DPAD_LEFT | DPAD_RIGHT) == DPAD_LEFT);

    SocdCleaner first(SocdMode::FirstWins);
    CHECK(first.clean(DPAD_LEFT) == DPAD_LEFT);
    CHECK(first.clean(DPAD_LEFT | DPAD_RIGHT) == DPAD_LEFT);

    SocdCleaner bypass(SocdMode::Bypass);
    CHECK(bypass.clean(DPAD_LEFT | DPAD_RIGHT) == (DPAD_LEFT | DPAD_RIGHT));
}

static void testDpadMode() {
    GamepadState s;
    s.dpad = DPAD_LEFT | DPAD_UP;
    applyDpadMode(s, DpadMode::LeftAnalog);
    CHECK(s.dpad == 0);
    CHECK(s.lx == AXIS_MIN);
    CHECK(s.ly == AXIS_MIN);
    CHECK(s.rx == AXIS_MID);
}

static void testAnalogFilter() {
    AnalogFilter f(1000, 300);
    CHECK(f.filter(AXIS_MID + 500) == AXIS_MID);    // inside deadzone
    CHECK(f.filter(AXIS_MID + 5000) == AXIS_MID + 5000);
    CHECK(f.filter(AXIS_MID + 5200) == AXIS_MID + 5000); // under hysteresis
    CHECK(f.filter(AXIS_MID + 5400) == AXIS_MID + 5400);
    CHECK(f.filter(AXIS_MAX) == AXIS_MAX);
}

static void testSwitchReport() {
    GamepadState s;
    s.buttons = BTN_B1 | BTN_A1;
    s.dpad = DPAD_DOWN | DPAD_RIGHT;
    uint8_t buf[MAX_REPORT_LENGTH];
    size_t n = buildReport(OutputMode::Switch, s, buf);
    CHECK(n == sizeof(SwitchReport));
    SwitchReport r;
    std::memcpy(&r, buf, sizeof(r));
    CHECK(r.buttons == (SWITCH_MASK_B | SWITCH_MASK_HOME));
    CHECK(r.hat == SWITCH_HAT_DOWNRIGHT);
    CHECK(r.lx == SWITCH_JOYSTICK_MID);

    GadgetIdentity id = gadgetIdentity(OutputMode::Switch);
    CHECK(id.vid == 0x0F0D);
    CHECK(id.pid == 0x0092);
    CHECK(id.reportLength == 8);
}

static void testDInputReport() {
    GamepadState s;
    s.buttons = BTN_B1 | BTN_S2;
    s.dpad = DPAD_UP;
    uint8_t buf[MAX_REPORT_LENGTH];
    size_t n = buildReport(OutputMode::DInput, s, buf);
    CHECK(n == sizeof(HIDReport));
    HIDReport r;
    std::memcpy(&r, buf, sizeof(r));
    CHECK(r.buttons == (BTN_B2 | BTN_S2 | (1u << 16)));
    CHECK(r.direction == HID_HAT_UP);
    GadgetIdentity id = gadgetIdentity(OutputMode::DInput);
    CHECK(id.vid == VENDOR_ID);
    CHECK(id.pid == PRODUCT_ID);
}

static void testSenderLatestOnly() {
    std::vector<std::vector<uint8_t>> written;
    bool busy = false;
    ReportSender sender([&](uint8_t const *d, size_t len) -> long {
        if (busy) {
            errno = EAGAIN;
            return -1;
        }
        written.emplace_back(d, d + len);
        return static_cast<long>(len);
    });
    uint8_t a[2] = {1, 0}, b[2] = {2, 0}, c[2] = {3, 0};

    CHECK(sender.submit(a, 2, 10));
    CHECK(!sender.submit(a, 2, 20));   // unchanged: nothing sent
    CHECK(written.size() == 1);

    busy = true;                       // a transfer is in flight
    CHECK(!sender.submit(b, 2, 30));
    CHECK(!sender.submit(c, 2, 40));   // b is replaced, never queued
    CHECK(sender.hasPending());
    CHECK(sender.replacedCount() == 1);
    busy = false;
    CHECK(sender.flush(50));
    CHECK(written.size() == 2);
    CHECK(written.back()[0] == 3);     // only the newest state went out

    busy = true;
    CHECK(!sender.submit(b, 2, 60));
    CHECK(!sender.submit(c, 2, 70));   // back to what the host has: drop it
    CHECK(!sender.hasPending());
    busy = false;
    CHECK(!sender.flush(80));
    CHECK(written.size() == 2);
}

static void testSenderHeartbeat() {
    int writes = 0;
    ReportSender sender([&](uint8_t const *, size_t len) -> long {
        writes++;
        return static_cast<long>(len);
    });
    sender.setHeartbeatNs(1000000); // 1 kHz
    uint8_t a[1] = {7};
    CHECK(!sender.tick(0));            // nothing to repeat yet
    sender.submit(a, 1, 1000);
    CHECK(writes == 1);
    CHECK(!sender.tick(500000));
    CHECK(sender.tick(1001000));
    CHECK(writes == 2);
    CHECK(sender.nextHeartbeat() == 2001000);
}

static void testConfig() {
    Config cfg;
    std::string err;
    CHECK(parseConfig("mode = dinput\npoll_rate_hz = 1000\npin.up = 5 # comment\n"
                      "pin.B1 = 6\nsocd = last_wins\n", cfg, err));
    CHECK(cfg.mode == OutputMode::DInput);
    CHECK(cfg.pollRateHz == 1000);
    CHECK(cfg.pins.size() == 2);
    CHECK(cfg.pins[0].dpad == DPAD_UP);
    CHECK(cfg.pins[1].button == BTN_B1);
    CHECK(cfg.socd == SocdMode::LastWins);

    Config bad;
    CHECK(!parseConfig("poll_rate_hz = 3000\n", bad, err));
    CHECK(!parseConfig("pin.b1 = 4\npin.b2 = 4\n", bad, err));
    CHECK(!parseConfig("pin.b1 = 99999999999999999999\n", bad, err));
    CHECK(!parseConfig("nonsense\n", bad, err));

    CHECK(highSpeedIntervalForRate(8000) == 1);
    CHECK(highSpeedIntervalForRate(1000) == 4);
    CHECK(highSpeedIntervalForRate(3000) == 0);
}

int main() {
    testDebounce();
    testSocd();
    testDpadMode();
    testAnalogFilter();
    testSwitchReport();
    testDInputReport();
    testSenderLatestOnly();
    testSenderHeartbeat();
    testConfig();
    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
