/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#include "gpio_input.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace gp2040linux {

std::string findHeaderGpioChip() {
    static char const *const kLabels[] = {"pinctrl-rp1", "pinctrl-bcm2712", "pinctrl-bcm2711",
                                          "pinctrl-bcm2835"};
    std::string best;
    size_t bestRank = sizeof(kLabels) / sizeof(kLabels[0]);
    DIR *dir = opendir("/dev");
    if (!dir) {
        return best;
    }
    while (dirent *e = readdir(dir)) {
        if (std::strncmp(e->d_name, "gpiochip", 8) != 0) {
            continue;
        }
        std::string p = std::string("/dev/") + e->d_name;
        int fd = ::open(p.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        gpiochip_info info{};
        if (ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info) == 0) {
            for (size_t i = 0; i < bestRank; i++) {
                if (std::strcmp(info.label, kLabels[i]) == 0) {
                    best = p;
                    bestRank = i;
                    break;
                }
            }
        }
        ::close(fd);
    }
    closedir(dir);
    return best;
}

GpioInput::~GpioInput() {
    if (lineFd >= 0) {
        ::close(lineFd);
    }
}

bool GpioInput::open(std::string const &chip, std::vector<PinMapping> const &pins,
                     bool activeLow, Bias bias, bool edgeEvents, std::string &error) {
    if (pins.empty()) {
        error = "no pins are mapped";
        return false;
    }
    if (pins.size() > GPIO_V2_LINES_MAX) {
        error = "too many pins for one GPIO request";
        return false;
    }
    path = (chip == "auto") ? findHeaderGpioChip() : chip;
    if (path.empty()) {
        error = "could not find the header GPIO chip; set gpio_chip in the config";
        return false;
    }
    int chipFd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (chipFd < 0) {
        error = "cannot open " + path + ": " + std::strerror(errno);
        return false;
    }

    gpio_v2_line_request req{};
    for (size_t i = 0; i < pins.size(); i++) {
        req.offsets[i] = pins[i].gpio;
    }
    req.num_lines = static_cast<uint32_t>(pins.size());
    std::strncpy(req.consumer, "gp2040-linux", sizeof(req.consumer) - 1);
    uint64_t flags = GPIO_V2_LINE_FLAG_INPUT;
    if (activeLow) flags |= GPIO_V2_LINE_FLAG_ACTIVE_LOW;
    if (bias == Bias::PullUp) flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_UP;
    else if (bias == Bias::PullDown) flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN;
    else flags |= GPIO_V2_LINE_FLAG_BIAS_DISABLED;
    if (edgeEvents) {
        flags |= GPIO_V2_LINE_FLAG_EDGE_RISING | GPIO_V2_LINE_FLAG_EDGE_FALLING;
        // Events only wake us up; the level itself is read with sample().
    }
    req.config.flags = flags;

    int rc = ioctl(chipFd, GPIO_V2_GET_LINE_IOCTL, &req);
    int savedErrno = errno;
    ::close(chipFd);
    if (rc < 0) {
        error = "GPIO line request on " + path + " failed: " + std::strerror(savedErrno) +
                " (is another program using these pins?)";
        return false;
    }
    lineFd = req.fd;
    count = req.num_lines;
    int fl = fcntl(lineFd, F_GETFL);
    fcntl(lineFd, F_SETFL, fl | O_NONBLOCK);
    return true;
}

void GpioInput::drainEvents() {
    gpio_v2_line_event events[16];
    while (::read(lineFd, events, sizeof(events)) > 0) {
    }
}

bool GpioInput::sample(uint64_t &mask) {
    gpio_v2_line_values values{};
    values.mask = (count >= 64) ? ~0ull : ((1ull << count) - 1);
    if (ioctl(lineFd, GPIO_V2_LINE_GET_VALUES_IOCTL, &values) < 0) {
        return false;
    }
    // ACTIVE_LOW is applied by the kernel, so 1 already means "pressed".
    mask = values.bits & values.mask;
    return true;
}

} // namespace gp2040linux
