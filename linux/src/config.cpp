/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
 */

#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace gp2040linux {

namespace {

std::string trim(std::string s) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool parseUnsigned(std::string const &v, unsigned &out) {
    if (v.empty() || v.size() > 9 || !std::all_of(v.begin(), v.end(), ::isdigit)) {
        return false;
    }
    out = static_cast<unsigned>(std::stoul(v));
    return true;
}

bool parseInt(std::string const &v, int &out) {
    if (v.empty() || v.size() > 9) {
        return false;
    }
    size_t start = (v[0] == '-') ? 1 : 0;
    if (start == v.size() || !std::all_of(v.begin() + start, v.end(), ::isdigit)) {
        return false;
    }
    out = std::stoi(v);
    return true;
}

bool parseBool(std::string const &v, bool &out) {
    if (v == "1" || v == "true" || v == "yes" || v == "on") { out = true; return true; }
    if (v == "0" || v == "false" || v == "no" || v == "off") { out = false; return true; }
    return false;
}

struct NamedInput {
    char const *name;
    uint32_t button;
    uint8_t dpad;
};

// Pin keys use the firmware's GP2040 button names.
NamedInput const kInputs[] = {
    {"up", 0, DPAD_UP},       {"down", 0, DPAD_DOWN},
    {"left", 0, DPAD_LEFT},   {"right", 0, DPAD_RIGHT},
    {"b1", BTN_B1, 0},        {"b2", BTN_B2, 0},
    {"b3", BTN_B3, 0},        {"b4", BTN_B4, 0},
    {"l1", BTN_L1, 0},        {"r1", BTN_R1, 0},
    {"l2", BTN_L2, 0},        {"r2", BTN_R2, 0},
    {"s1", BTN_S1, 0},        {"s2", BTN_S2, 0},
    {"l3", BTN_L3, 0},        {"r3", BTN_R3, 0},
    {"a1", BTN_A1, 0},        {"a2", BTN_A2, 0},
    {"a3", BTN_A3, 0},        {"a4", BTN_A4, 0},
};

} // namespace

unsigned highSpeedIntervalForRate(unsigned hz) {
    switch (hz) {
        case 8000: return 1;
        case 4000: return 2;
        case 2000: return 3;
        case 1000: return 4;
        case 500: return 5;
        case 250: return 6;
        case 125: return 7;
        default: return 0;
    }
}

bool parseConfig(std::string const &text, Config &cfg, std::string &error) {
    std::istringstream in(text);
    std::string line;
    int lineNo = 0;
    std::vector<unsigned> usedGpios;
    while (std::getline(in, line)) {
        lineNo++;
        auto hash = line.find('#');
        if (hash != std::string::npos) {
            line.erase(hash);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) {
            error = "line " + std::to_string(lineNo) + ": expected key = value";
            return false;
        }
        std::string key = lower(trim(line.substr(0, eq)));
        std::string value = trim(line.substr(eq + 1));
        std::string lv = lower(value);
        auto bad = [&](char const *what) {
            error = "line " + std::to_string(lineNo) + ": " + what + " for '" + key + "'";
            return false;
        };

        if (key == "mode") {
            if (!parseOutputMode(lv, cfg.mode)) return bad("unknown mode (switch, dinput)");
        } else if (key == "poll_rate_hz") {
            if (!parseUnsigned(lv, cfg.pollRateHz) || highSpeedIntervalForRate(cfg.pollRateHz) == 0)
                return bad("use 8000, 4000, 2000, 1000, 500, 250 or 125");
        } else if (key == "heartbeat_hz") {
            if (!parseUnsigned(lv, cfg.heartbeatHz) || cfg.heartbeatHz > 8000)
                return bad("use 0..8000");
        } else if (key == "debounce_us") {
            if (!parseUnsigned(lv, cfg.debounceUs) || cfg.debounceUs > 100000)
                return bad("use 0..100000");
        } else if (key == "socd") {
            if (lv == "up_priority") cfg.socd = SocdMode::UpPriority;
            else if (lv == "neutral") cfg.socd = SocdMode::Neutral;
            else if (lv == "last_wins" || lv == "second_input_priority") cfg.socd = SocdMode::LastWins;
            else if (lv == "first_wins" || lv == "first_input_priority") cfg.socd = SocdMode::FirstWins;
            else if (lv == "bypass" || lv == "off") cfg.socd = SocdMode::Bypass;
            else return bad("use up_priority, neutral, last_wins, first_wins or bypass");
        } else if (key == "dpad_mode") {
            if (lv == "digital") cfg.dpadMode = DpadMode::Digital;
            else if (lv == "left_analog") cfg.dpadMode = DpadMode::LeftAnalog;
            else if (lv == "right_analog") cfg.dpadMode = DpadMode::RightAnalog;
            else return bad("use digital, left_analog or right_analog");
        } else if (key == "input_method") {
            if (lv == "events") cfg.inputMethod = InputMethod::Events;
            else if (lv == "busypoll") cfg.inputMethod = InputMethod::BusyPoll;
            else return bad("use events or busypoll");
        } else if (key == "busypoll_interval_us") {
            if (!parseUnsigned(lv, cfg.busyPollIntervalUs)) return bad("expected a number");
        } else if (key == "cpu") {
            if (!parseInt(lv, cfg.cpu) || cfg.cpu < -1) return bad("expected a CPU number or -1");
        } else if (key == "rt_priority") {
            if (!parseInt(lv, cfg.rtPriority) || cfg.rtPriority < 0 || cfg.rtPriority > 99)
                return bad("use 0..99");
        } else if (key == "gpio_chip") {
            cfg.gpioChip = value;
        } else if (key == "hidg_device") {
            cfg.hidgDevice = value;
        } else if (key == "active_low") {
            if (!parseBool(lv, cfg.activeLow)) return bad("expected true or false");
        } else if (key == "bias") {
            if (lv == "pull_up") cfg.bias = Bias::PullUp;
            else if (lv == "pull_down") cfg.bias = Bias::PullDown;
            else if (lv == "none") cfg.bias = Bias::None;
            else return bad("use pull_up, pull_down or none");
        } else if (key.rfind("pin.", 0) == 0) {
            std::string name = key.substr(4);
            auto it = std::find_if(std::begin(kInputs), std::end(kInputs),
                                   [&](NamedInput const &n) { return name == n.name; });
            if (it == std::end(kInputs)) return bad("unknown input name");
            unsigned gpio;
            if (!parseUnsigned(lv, gpio) || gpio > 63) return bad("expected a GPIO number 0..63");
            if (std::find(usedGpios.begin(), usedGpios.end(), gpio) != usedGpios.end())
                return bad("GPIO already used by another input");
            usedGpios.push_back(gpio);
            cfg.pins.push_back({gpio, it->button, it->dpad});
        } else {
            return bad("unknown key");
        }
    }
    if (cfg.pins.size() > 64) {
        error = "at most 64 pins can be mapped";
        return false;
    }
    return true;
}

bool loadConfig(std::string const &path, Config &cfg, std::string &error) {
    std::ifstream f(path);
    if (!f) {
        error = "cannot open " + path;
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return parseConfig(ss.str(), cfg, error);
}

} // namespace gp2040linux
