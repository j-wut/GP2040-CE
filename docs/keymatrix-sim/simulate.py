#!/usr/bin/env python3
"""
Electrical simulation (ngspice) of the keyboard matrix wiring on an RP2040.

Models a 3 x 3 corner of the matrix, which is enough to cover every interaction
the full 6 x 8 matrix has: a scanned row, the row scanned just before it, and
an idle row, each crossing three columns.

  - every row and column wire: an RP2040 internal pull-up to 3.3 V plus wire
    and pin capacitance to ground
  - the row being scanned: pulled to ground by the GPIO output driver
  - every switch: a 1N4148 from the column (anode) to the row (cathode, stripe)
    in series with the switch contacts

The firmware scans rows one after another: row k is pulled low for the settle
time, the columns are read at the end of that window, then the row is released
and the firmware waits the row delay before the next row is pulled low. Both
times are set in the web config.

For each scenario the script checks, at every read:
  - a pressed switch reads low: column below the RP2040 VIL (0.8 V)
  - an open switch reads high: column above the RP2040 VIH (2.0 V)
A column between the two is a failed read (undefined logic level).

Usage: python3 simulate.py   (needs ngspice on PATH)
"""
import itertools
import os
import re
import subprocess
import sys
import tempfile

VDD = 3.3
VIL = 0.8   # RP2040 datasheet, IOVDD = 3.3 V
VIH = 2.0
ROWS, COLS = 3, 3
SCAN_CYCLES = 2  # scan the rows twice so the first row also follows the last

# 1N4148 (standard vendor SPICE model)
DIODE_MODEL = ".model D1N4148 D(Is=2.52n Rs=0.568 N=1.752 Cjo=4p M=0.4 Vj=0.5 tt=20n Bv=100 Ibv=100u)"

# what is held down in each scenario: set of (row, col)
SCENARIOS = {
    "single switch": {(0, 0)},
    "ghost rectangle (3 corners held)": {(0, 0), (0, 1), (1, 0)},
    "whole row held": {(0, 0), (0, 1), (0, 2)},
    "whole column held": {(0, 0), (1, 0), (2, 0)},
    "everything held (NKRO)": set(itertools.product(range(ROWS), range(COLS))),
    "diagonal": {(0, 0), (1, 1), (2, 2)},
}


def netlist(pressed, pullup, cap, settle_us, ron, gap_us=0.0):
    t = settle_us * 1e-6
    g = gap_us * 1e-6
    w = t + g  # one row: pulled low for the settle time, then the recovery wait
    lines = [f"* matrix pullup={pullup} cap={cap} settle={settle_us}us", DIODE_MODEL,
             "VDD vdd 0 DC 3.3",
             ".model SWGPIO SW(Ron=%g Roff=1e12 Vt=0.5 Vh=0)" % ron]
    for r in range(ROWS):
        lines += [f"RPU_R{r} vdd row{r} {pullup}", f"CR{r} row{r} 0 {cap}"]
        # GPIO output driver: closes for every window where this row is scanned
        pwl = ["0 0"]
        for cyc in range(SCAN_CYCLES):
            start = (cyc * ROWS + r) * w
            end = start + t
            pwl += [f"{start + 1e-9:.12g} 0", f"{start + 2e-9:.12g} 1",
                    f"{end - 1e-9:.12g} 1", f"{end:.12g} 0"]
        lines += [f"VDRV{r} drv{r} 0 PWL({' '.join(pwl)})",
                  f"SDRV{r} row{r} 0 drv{r} 0 SWGPIO"]
    for c in range(COLS):
        lines += [f"RPU_C{c} vdd col{c} {pullup}", f"CC{c} col{c} 0 {cap}"]
    for (r, c) in pressed:
        lines.append(f"D{r}{c} col{c} row{r} D1N4148")
    end = SCAN_CYCLES * ROWS * w
    lines += [f".tran {min(t, w) / 200:.6g} {end:.6g}", ".control", "run"]
    # read every column at the end of every scan window (just before release)
    for cyc in range(SCAN_CYCLES):
        for r in range(ROWS):
            sample = (cyc * ROWS + r) * w + t - 5e-9
            for c in range(COLS):
                lines.append(f"meas tran m_{cyc}_{r}_{c} FIND v(col{c}) AT={sample:.12g}")
    lines += [".endc", ".end"]
    return "\n".join(lines)


def run(pressed, pullup, cap, settle_us, ron, gap_us=0.0):
    with tempfile.NamedTemporaryFile("w", suffix=".cir", delete=False) as f:
        f.write(netlist(pressed, pullup, cap, settle_us, ron, gap_us))
        path = f.name
    try:
        out = subprocess.run(["ngspice", "-b", path], capture_output=True, text=True).stdout
    finally:
        os.unlink(path)
    values = {}
    for m in re.finditer(r"m_(\d+)_(\d+)_(\d+)\s*=\s*([-+0-9.eE]+)", out):
        values[tuple(int(x) for x in m.groups()[:3])] = float(m.group(4))
    if len(values) != SCAN_CYCLES * ROWS * COLS:
        sys.exit("ngspice did not return every measurement:\n" + out[-2000:])
    return values


def check(pressed, values):
    """Worst low for pressed reads, worst high for open reads, failures."""
    worst_low, worst_high, failures = 0.0, VDD, []
    for (cyc, r, c), v in values.items():
        if (r, c) in pressed:
            worst_low = max(worst_low, v)
            if v > VIL:
                failures.append(f"R{r}C{c} pressed read {v:.2f} V")
        else:
            worst_high = min(worst_high, v)
            if v < VIH:
                failures.append(f"R{r}C{c} open read {v:.2f} V")
    return worst_low, worst_high, failures


def recovery_time(pressed, pullup, cap, settle_us, ron, gap_us=400.0):
    """Worst time after a row is released until every column is back above VIH.

    This is how long the firmware's wait-for-columns-high loop takes."""
    text = netlist(pressed, pullup, cap, settle_us, ron, gap_us)
    meas = []
    for r in range(ROWS):
        release = (r * (settle_us + gap_us) + settle_us) * 1e-6
        for c in range(COLS):
            if (r, c) in pressed:  # only columns this row pulled low need to recover
                meas.append(f"meas tran rec_{r}_{c} WHEN v(col{c})={VIH} RISE=1 FROM={release:.12g}")
    text = text.replace(".endc", "\n".join(meas) + "\n.endc")
    with tempfile.NamedTemporaryFile("w", suffix=".cir", delete=False) as f:
        f.write(text)
        path = f.name
    try:
        out = subprocess.run(["ngspice", "-b", path], capture_output=True, text=True).stdout
    finally:
        os.unlink(path)
    worst = 0.0
    for m in re.finditer(r"rec_(\d+)_(\d+)\s*=\s*([-+0-9.eE]+)", out):
        r, c = int(m.group(1)), int(m.group(2))
        release = (r * (settle_us + gap_us) + settle_us) * 1e-6
        worst = max(worst, float(m.group(3)) - release)
    return worst * 1e6


PULLUPS = [50e3, 80e3]       # RP2040 internal pull-up range
CAPS = [20e-12, 100e-12, 200e-12, 500e-12, 1000e-12]  # per wire: pin + PCB up to a long harness
RON = 40                     # GPIO output low resistance at 3.3 V
SETTLE_US = 5                # KEY_MATRIX_SETTLE_MICROS
ROW_DELAYS_US = [0, 10, 20, 40, 70, 100]  # KEY_MATRIX_ROW_DELAY_MICROS (default 40), set in the web config
DEFAULT_ROW_DELAY_US = 40


def row_delay_table():
    print(f"Settle {SETTLE_US} us, then the row delay after each row is released")
    print(f"{'pull-up':>8} {'wire C':>7}  {'recovery':>9}  " + "  ".join(f"{d:>4}us" for d in ROW_DELAYS_US))
    default_ok = {}
    for pullup, cap in itertools.product(PULLUPS, CAPS):
        rec = max(recovery_time(p, pullup, cap, SETTLE_US, RON) for p in SCENARIOS.values())
        cells = []
        for delay in ROW_DELAYS_US:
            fails = []
            for name, pressed in SCENARIOS.items():
                _, _, f = check(pressed, run(pressed, pullup, cap, SETTLE_US, RON, delay))
                fails += f
            cells.append("PASS" if not fails else "FAIL")
            if delay == DEFAULT_ROW_DELAY_US:
                default_ok[(pullup, cap)] = not fails
        print(f"{pullup / 1e3:>6.0f}k {cap * 1e12:>5.0f}pF  {rec:>6.1f} us  " + "  ".join(f"{c:>6}" for c in cells))
    print("\nrecovery: time after a row is released until its columns read high again.")
    print("A row delay longer than the recovery always passes. Every read is checked for every pattern.")
    return default_ok


def main():
    print(f"Pass = pressed reads < {VIL} V and open reads > {VIH} V at every read\n")
    row_delay_table()


if __name__ == "__main__":
    main()
