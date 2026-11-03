/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

// gp2040-linux: GP2040-CE controller logic for Linux boards with a USB device
// (gadget) port, such as the Raspberry Pi 5, CM5, Pi 4 and CM4.
//
// Buttons are read from GPIO, cleaned (debounce, SOCD, d-pad mode), turned into
// the same full-state HID reports the firmware sends, and written to the
// kernel's f_hid gadget (/dev/hidg0) only when they change.

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#include "config.h"
#include "debounce.h"
#include "gamepad_logic.h"
#include "gpio_input.h"
#include "report_builder.h"
#include "report_sender.h"

using namespace gp2040linux;

namespace {

volatile sig_atomic_t gStop = 0;

void onSignal(int) { gStop = 1; }

uint64_t nowNs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + static_cast<uint64_t>(ts.tv_nsec);
}

void usage(char const *argv0) {
    std::fprintf(stderr,
        "usage: %s [--config FILE] [--dry-run] [--print-gadget-env] [--print-report-desc]\n"
        "\n"
        "  --config FILE        settings (default /etc/gp2040-linux.conf)\n"
        "  --dry-run            print reports to stdout instead of writing to the gadget\n"
        "  --print-gadget-env   print shell variables used by gp2040-gadget.sh\n"
        "  --print-report-desc  write the binary HID report descriptor to stdout\n",
        argv0);
}

void printGadgetEnv(Config const &cfg) {
    GadgetIdentity id = gadgetIdentity(cfg.mode);
    std::printf("GADGET_MODE=%s\n", outputModeName(cfg.mode));
    std::printf("GADGET_VID=0x%04x\n", id.vid);
    std::printf("GADGET_PID=0x%04x\n", id.pid);
    std::printf("GADGET_BCD_DEVICE=0x%04x\n", id.bcdDevice);
    std::printf("GADGET_MANUFACTURER='%s'\n", id.manufacturer);
    std::printf("GADGET_PRODUCT='%s'\n", id.product);
    std::printf("GADGET_REPORT_LENGTH=%zu\n", id.reportLength);
    std::printf("GADGET_POLL_RATE_HZ=%u\n", cfg.pollRateHz);
    std::printf("GADGET_HS_INTERVAL=%u\n", highSpeedIntervalForRate(cfg.pollRateHz));
}

void setupRealtime(Config const &cfg) {
    if (cfg.cpu >= 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cfg.cpu, &set);
        if (sched_setaffinity(0, sizeof(set), &set) != 0) {
            std::fprintf(stderr, "warning: could not pin to CPU %d: %s\n", cfg.cpu,
                         std::strerror(errno));
        }
    }
    if (cfg.rtPriority > 0) {
        sched_param sp{};
        sp.sched_priority = cfg.rtPriority;
        if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
            std::fprintf(stderr, "warning: could not enable SCHED_FIFO: %s\n",
                         std::strerror(errno));
        }
        // Avoid page-fault stalls in the input path.
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
            std::fprintf(stderr, "warning: mlockall failed: %s\n", std::strerror(errno));
        }
    }
}

class Controller {
public:
    explicit Controller(Config const &c) : cfg(c), debouncer(uint64_t(c.debounceUs) * 1000u),
                                           socd(c.socd) {}

    // Turn a debounced pin mask into a report and offer it to the sender.
    void update(uint64_t pinMask, uint64_t now, ReportSender &sender) {
        uint64_t stable = debouncer.update(pinMask, now);
        GamepadState s;
        uint8_t dpad = 0;
        for (size_t i = 0; i < cfg.pins.size(); i++) {
            if (stable & (1ull << i)) {
                s.buttons |= cfg.pins[i].button;
                dpad |= cfg.pins[i].dpad;
            }
        }
        s.dpad = socd.clean(dpad);
        applyDpadMode(s, cfg.dpadMode);
        uint8_t report[MAX_REPORT_LENGTH];
        size_t len = buildReport(cfg.mode, s, report);
        sender.submit(report, len, now);
    }

    uint64_t debounceDeadline() const { return debouncer.nextDeadline(); }

private:
    Config const &cfg;
    Debouncer debouncer;
    SocdCleaner socd;
};

int openHidg(std::string const &path) {
    return ::open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
}

void drainOutReports(int fd) {
    // Host-to-device reports (LEDs, rumble) are not used yet; read them so
    // they do not pile up in the kernel.
    uint8_t buf[64];
    while (::read(fd, buf, sizeof(buf)) > 0) {
    }
}

} // namespace

int main(int argc, char **argv) {
    std::string configPath = "/etc/gp2040-linux.conf";
    bool dryRun = false;
    bool printEnv = false;
    bool printDesc = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--config" && i + 1 < argc) configPath = argv[++i];
        else if (a == "--dry-run") dryRun = true;
        else if (a == "--print-gadget-env") printEnv = true;
        else if (a == "--print-report-desc") printDesc = true;
        else { usage(argv[0]); return 2; }
    }

    Config cfg;
    std::string error;
    if (!loadConfig(configPath, cfg, error)) {
        std::fprintf(stderr, "config error: %s\n", error.c_str());
        return 1;
    }
    if (printEnv) {
        printGadgetEnv(cfg);
        return 0;
    }
    if (printDesc) {
        GadgetIdentity id = gadgetIdentity(cfg.mode);
        return std::fwrite(id.reportDescriptor, 1, id.reportDescriptorLength, stdout) ==
                       id.reportDescriptorLength ? 0 : 1;
    }

    GpioInput gpio;
    bool const events = cfg.inputMethod == InputMethod::Events;
    if (!gpio.open(cfg.gpioChip, cfg.pins, cfg.activeLow, cfg.bias, events, error)) {
        std::fprintf(stderr, "gpio error: %s\n", error.c_str());
        return 1;
    }

    int hidFd = -1;
    ReportSender sender([&](uint8_t const *data, size_t len) -> long {
        if (dryRun) {
            for (size_t i = 0; i < len; i++) std::printf("%02x", data[i]);
            std::printf("\n");
            std::fflush(stdout);
            return static_cast<long>(len);
        }
        if (hidFd < 0) {
            errno = ENODEV;
            return -1;
        }
        return ::write(hidFd, data, len);
    });
    if (cfg.heartbeatHz > 0) {
        sender.setHeartbeatNs(1000000000ull / cfg.heartbeatHz);
    }

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    setupRealtime(cfg);

    std::fprintf(stderr,
                 "gp2040-linux: mode=%s poll_rate_hz=%u (bInterval %u on High Speed) "
                 "input=%s gpio=%s pins=%zu%s\n",
                 outputModeName(cfg.mode), cfg.pollRateHz, highSpeedIntervalForRate(cfg.pollRateHz),
                 events ? "events" : "busypoll", gpio.chipPath().c_str(), cfg.pins.size(),
                 dryRun ? " dry-run" : "");

    Controller controller(cfg);
    uint64_t retryAt = 0; // back-off after a gadget error so we do not spin
    uint64_t pins = 0;
    if (gpio.sample(pins)) {
        controller.update(pins, nowNs(), sender);
    }

    while (!gStop) {
        uint64_t now = nowNs();
        if (!dryRun && hidFd < 0 && now >= retryAt) {
            hidFd = openHidg(cfg.hidgDevice);
            if (hidFd < 0) {
                retryAt = now + 500000000ull; // gadget not bound yet
            }
        }

        if (!events) {
            if (gpio.sample(pins)) {
                controller.update(pins, now, sender);
            }
            if (sender.hasPending() && now >= retryAt) {
                uint64_t errs = sender.errorCount();
                sender.flush(now);
                if (sender.errorCount() != errs) retryAt = now + 10000000ull;
            }
            sender.tick(now);
            if (hidFd >= 0) drainOutReports(hidFd);
            if (cfg.busyPollIntervalUs > 0) {
                timespec ts{0, static_cast<long>(cfg.busyPollIntervalUs) * 1000L};
                clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
            }
            continue;
        }

        // Event-driven: sleep until an edge, a debounce window ends, the
        // device can take the held report, or the heartbeat is due.
        uint64_t deadline = 0;
        auto consider = [&](uint64_t t) {
            if (t != 0 && (deadline == 0 || t < deadline)) deadline = t;
        };
        consider(controller.debounceDeadline());
        consider(sender.nextHeartbeat());
        if (retryAt > now) consider(retryAt);
        else if (!dryRun && hidFd < 0) consider(now + 500000000ull);

        pollfd fds[2];
        int nfds = 0;
        fds[nfds++] = {gpio.eventFd(), POLLIN, 0};
        int hidIndex = -1;
        if (hidFd >= 0) {
            short want = POLLIN;
            if (sender.hasPending() && now >= retryAt) want |= POLLOUT;
            hidIndex = nfds;
            fds[nfds++] = {hidFd, want, 0};
        } else if (dryRun && sender.hasPending()) {
            sender.flush(now);
        }

        timespec ts{};
        timespec *tsp = nullptr;
        if (deadline != 0) {
            uint64_t wait = deadline > now ? deadline - now : 0;
            ts.tv_sec = static_cast<time_t>(wait / 1000000000ull);
            ts.tv_nsec = static_cast<long>(wait % 1000000000ull);
            tsp = &ts;
        }
        int rc = ppoll(fds, nfds, tsp, nullptr);
        if (rc < 0 && errno != EINTR) {
            std::perror("ppoll");
            break;
        }
        now = nowNs();

        bool resample = (controller.debounceDeadline() != 0 && now >= controller.debounceDeadline());
        if (rc > 0 && (fds[0].revents & POLLIN)) {
            gpio.drainEvents();
            resample = true;
        }
        if (resample && gpio.sample(pins)) {
            controller.update(pins, now, sender);
        }

        if (hidIndex >= 0 && rc > 0) {
            short re = fds[hidIndex].revents;
            if (re & POLLIN) drainOutReports(hidFd);
            if (re & POLLOUT) {
                uint64_t errs = sender.errorCount();
                sender.flush(now);
                if (sender.errorCount() != errs) retryAt = now + 10000000ull;
            }
            if (re & (POLLERR | POLLHUP | POLLNVAL)) {
                ::close(hidFd);
                hidFd = -1;
                retryAt = now + 100000000ull;
            }
        }
        if (now >= retryAt) {
            uint64_t errs = sender.errorCount();
            sender.tick(now);
            if (sender.errorCount() != errs) retryAt = now + 10000000ull;
        }
    }

    std::fprintf(stderr, "gp2040-linux: sent=%llu busy=%llu replaced=%llu errors=%llu\n",
                 static_cast<unsigned long long>(sender.sentCount()),
                 static_cast<unsigned long long>(sender.busyCount()),
                 static_cast<unsigned long long>(sender.replacedCount()),
                 static_cast<unsigned long long>(sender.errorCount()));
    if (hidFd >= 0) ::close(hidFd);
    return 0;
}
