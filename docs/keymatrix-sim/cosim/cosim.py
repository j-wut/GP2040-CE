#!/usr/bin/env python3
"""Co-simulation: the firmware's key matrix scan against the diode adapter's circuit model.

The real src/keymatrix.cpp is compiled for the host (harness.cpp) with mocked
GPIO and a virtual clock. For each test:

  1. The firmware scans with every read seeing all pins high. That records the
     exact times it pulls each row low, releases it, and reads the columns.
     Reads never change the firmware's timing, so the schedule is fixed.
  2. ngspice simulates the adapter matrix (the model from the adapter's
     simulate.py: RP2040 pins and pull-ups, ribbon, PCB copper, diodes,
     harness wiring) with the rows driven on exactly that schedule, and
     samples every column at every read instant.
  3. The firmware scans again, each read returning the simulated column levels
     (low below 0.8 V, high above 2.0 V; anything between fails the test, since
     the RP2040 input is undefined there).
  4. The firmware's switch state after each scan must equal the held switches.

Tests: every switch alone, multi-key patterns (ghost rectangles, rows,
columns, all 44, random sets), and a row delay sweep, for each wiring length
and both pull-up extremes.

Usage: python3 cosim.py PATH/TO/gp2040-hardware/"Boards/Supported Boards/Key Matrix Diode Adapter"
Needs g++, ngspice, and (for PCB copper) KiCad's Python module.
"""

import argparse
import concurrent.futures as cf
import itertools
import os
import random
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SCANS = 3          # steady state from the second scan on; all three are checked


def build_harness(out):
    subprocess.run(["g++", "-std=c++17", "-O1", "-Wall", "-I", os.path.join(HERE, "stubs"),
                    "-I", os.path.join(REPO, "headers"), "-o", out, os.path.join(HERE, "harness.cpp"),
                    os.path.join(REPO, "src", "keymatrix.cpp")], check=True)


def run_harness(exe, rows, cols, settle, delay, reads=None):
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
        if reads is not None:
            f.write("\n".join(f"{v:x}" for v in reads))
        path = f.name
    try:
        args = [exe, ",".join(map(str, rows)), ",".join(map(str, cols)), str(settle), str(delay), str(SCANS)]
        out = subprocess.run(args + ([path] if reads is not None else []), capture_output=True,
                             text=True, check=True).stdout
    finally:
        os.unlink(path)
    pins, drives, read_times, states = [], [], [], []
    for line in out.splitlines():
        f = line.split()
        if f[0] == "pins":
            pins = [int(x) for x in f[1:]]
        elif f[0] == "drive":
            drives.append((int(f[1]), float(f[2]) * 1e-6, int(f[3])))
        elif f[0] == "read":
            read_times.append(float(f[2]) * 1e-6)
        elif f[0] == "state":
            states.append(int(f[2], 16))
    return pins, drives, read_times, states


def netlist(sim, pressed, wiring, pullup, caps, drives, read_times, end):
    """The adapter's own netlist (simulate.py), rows driven on the firmware's schedule."""
    lows, start = {}, {}
    for pin, t, out in drives:
        if pin in start and not out:
            lows.setdefault(sim.generate.ROW_GPIOS.index(pin), []).append((start.pop(pin), t))
        elif out:
            start[pin] = t
    for pin, t in start.items():
        lows.setdefault(sim.generate.ROW_GPIOS.index(pin), []).append((t, end))
    L = sim.netlist(pressed, wiring, pullup, caps, lows, end)
    for n, t in enumerate(read_times):
        for c in range(len(sim.generate.COL_GPIOS)):
            # the input is sampled during the read's register access
            L.append(f"meas tran v_{n}_{c} FIND v(cm{c}) AT={t - 1e-9:.12g}")
    return L + [".endc", ".end"]


def measure(sim, lines, nreads, ncols):
    out = sim.run_ngspice(lines)
    vals = {(int(m.group(1)), int(m.group(2))): float(m.group(3))
            for m in re.finditer(r"^v_(\d+)_(\d+)\s*=\s*([-+0-9.eE]+)", out, re.M)}
    if len(vals) != nreads * ncols:
        raise RuntimeError("ngspice did not return every reading:\n" + out[-2000:])
    return vals


def run_case(ctx, pressed, wiring, pullup, settle, delay):
    """One co-simulation. Returns a list of failures (empty when the firmware got it right)."""
    sim, gen, exe, caps = ctx["sim"], ctx["gen"], ctx["exe"], ctx["caps"]
    rows, cols = gen.ROW_GPIOS, gen.COL_GPIOS
    pins, drives, read_times, _ = run_harness(exe, rows, cols, settle, delay)
    end = read_times[-1] + 2e-6
    vals = measure(sim, netlist(sim, pressed, wiring, pullup, caps, drives, read_times, end),
                   len(read_times), len(cols))
    fails, reads = [], []
    for n in range(len(read_times)):
        word = ~0 & ((1 << 64) - 1)
        for c, gpio in enumerate(cols):
            v = vals[(n, c)]
            if sim.VIL <= v <= sim.VIH:
                fails.append(f"read {n} column {c + 1}: {v:.2f} V is between the input thresholds")
            elif v < sim.VIL:
                word &= ~(1 << gpio)
        reads.append(word)
    _, _, _, states = run_harness(exe, rows, cols, settle, delay, reads)
    want = 0
    for r, c in pressed:
        want |= 1 << pins[r * len(cols) + c]
    for s, got in enumerate(states):
        if got != want:
            extra = [k for k in range(len(pins)) if (got >> pins[k]) & 1 and not (want >> pins[k]) & 1]
            missed = [k for k in range(len(pins)) if (want >> pins[k]) & 1 and not (got >> pins[k]) & 1]
            name = lambda k: f"R{k // len(cols) + 1}C{k % len(cols) + 1}"
            fails.append(f"scan {s + 1}: " + ", ".join(
                [f"missed {name(k)}" for k in missed] + [f"phantom {name(k)}" for k in extra]))
    return fails


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("adapter", help="the Key Matrix Diode Adapter folder in gp2040-hardware")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--random", type=int, default=20, help="random multi-key sets per wiring case")
    args = ap.parse_args()

    sys.path.insert(0, os.path.abspath(args.adapter))
    import generate as gen  # noqa: E402  (the adapter's own design table)
    import simulate as sim  # noqa: E402  (the adapter's circuit model)

    # the firmware board config must match the adapter
    cfg = open(os.path.join(REPO, "configs", "RP2040AdvancedBreakoutBoardUSBPassthroughMatrix", "BoardConfig.h")).read()
    fw = lambda key: [int(v) for v in re.search(rf"#define {key} ([\d, ]+)", cfg).group(1).split(",")]
    settle = int(re.search(r"#define KEY_MATRIX_SETTLE_MICROS (\d+)", open(os.path.join(REPO, "headers", "keymatrix.h")).read()).group(1))
    delay = int(re.search(r"#define KEY_MATRIX_ROW_DELAY_MICROS (\d+)", open(os.path.join(REPO, "headers", "keymatrix.h")).read()).group(1))
    assert fw("KEY_MATRIX_ROW_PINS") == gen.ROW_GPIOS and fw("KEY_MATRIX_COL_PINS") == gen.COL_GPIOS, \
        "firmware board config and adapter disagree on the matrix pins"

    exe = os.path.join(tempfile.mkdtemp(), "harness")
    build_harness(exe)
    caps, note = sim.pcb_caps()
    ctx = {"sim": sim, "gen": gen, "exe": exe, "caps": caps}
    R, C = len(gen.ROW_GPIOS), len(gen.COL_GPIOS)
    allsw = list(itertools.product(range(R), range(C)))
    print(f"Firmware src/keymatrix.cpp against the adapter model: {R} rows (GPIO {gen.ROW_GPIOS[0]}-{gen.ROW_GPIOS[-1]}), "
          f"{C} columns (GPIO {gen.COL_GPIOS[0]}-{gen.COL_GPIOS[-1]}), defaults settle {settle} us, row delay {delay} us")
    if note:
        print(note)

    rng = random.Random(2040)
    patterns = {f"single {f'R{r + 1}C{c + 1}'}": {(r, c)} for r, c in allsw}
    patterns.update({k: v for k, v in sim.SCENARIOS.items() if k != "single switch"})
    patterns["every ghost rectangle corner set"] = {(0, 0), (0, C - 1), (R - 1, 0)}
    patterns["all but one"] = set(allsw) - {(1, 1)}
    for i in range(args.random):
        patterns[f"random {i + 1}"] = set(rng.sample(allsw, rng.randint(2, len(allsw) - 1)))

    ok = True
    print(f"\n{len(patterns)} switch patterns per wiring case: every switch alone, NKRO/ghosting patterns, "
          f"{args.random} random sets. {SCANS} scans each, every scan checked.\n")
    print(f"{'wiring (ribbon / switch wire / column chain)':<48} {'pull-up':>7}  {'patterns':>8}  result")
    with cf.ThreadPoolExecutor(args.jobs) as pool:
        for wiring, pullup in itertools.product(sim.WIRING, sim.PULLUPS):
            futs = {pool.submit(run_case, ctx, p, wiring, pullup, settle, delay): n for n, p in patterns.items()}
            fails = []
            for f in cf.as_completed(futs):
                fails += [f"{futs[f]}: {x}" for x in f.result()]
            ok &= not fails
            label = wiring[0] if wiring[1] == 0 else f"{wiring[0]} ({wiring[1]:g} / {wiring[2]:g} / {wiring[3]:g} m)"
            print(f"{label:<48} {pullup / 1e3:>5.0f}k  {len(patterns):>8}  " +
                  ("PASS" if not fails else f"FAIL ({len(fails)}): " + "; ".join(sorted(fails)[:3])), flush=True)

        # row delay: the shortest firmware row delay that still reads every
        # pattern right, per wiring, with the worst-case pull-up
        print(f"\nRow delay sweep (80k pull-up, NKRO patterns): shortest row delay with every read right")
        sweep = [0, 2, 5, 10, 15, 20, 25, 30, 35, 40, 50]
        hard = {k: sim.SCENARIOS[k] for k in ("every other row held", "everything held", "ghost rectangle", "checkerboard")}
        for wiring in sim.WIRING:
            futs = {pool.submit(run_case, ctx, p, wiring, 80e3, settle, d): d for d in sweep for p in hard.values()}
            bad = {futs[f] for f in cf.as_completed(futs) if f.result()}
            good = [d for d in sweep if d not in bad and all(x not in bad for x in sweep if x > d)]
            label = wiring[0] if wiring[1] == 0 else f"{wiring[0]} ({wiring[1]:g} / {wiring[2]:g} / {wiring[3]:g} m)"
            shortest = f"{min(good)} us" if good else "none of the tested delays"
            print(f"  {label:<48} {shortest:>8}   (default {delay} us: {'ok' if delay in good else 'FAILS'})")
            ok &= delay in good
    print("\nAll co-simulation checks passed" if ok else "\nCo-simulation found failures")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
