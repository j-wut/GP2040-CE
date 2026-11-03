#!/usr/bin/env python3
"""Check the adapter design and write the manufacturing files.

1. KiCad DRC on the PCB (clearances, unconnected pads, ...). Must be clean.
2. The schematic netlist (exported by kicad-cli) must connect exactly the same
   pads as the PCB.
3. The matrix must match the firmware board config (rows, columns, diodes).
4. Writes gerbers, drill, pick-and-place, BOM and a schematic PDF to fab/.
"""

import csv
import os
import re
import subprocess
import sys

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import generate  # noqa: E402

NAME = generate.NAME
PCB = os.path.join(HERE, NAME + ".kicad_pcb")
SCH = os.path.join(HERE, NAME + ".kicad_sch")
BUILD = os.path.join(HERE, "build")
FAB = os.path.join(HERE, "fab")
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))

errors = []


def fail(msg):
    errors.append(msg)
    print("FAIL:", msg)


def drc():
    os.makedirs(BUILD, exist_ok=True)
    report = os.path.join(BUILD, "drc.rpt")
    board = pcbnew.LoadBoard(PCB)
    pcbnew.WriteDRCReport(board, report, pcbnew.EDA_UNITS_MILLIMETRES, True)
    text = open(report).read()
    # Library-table warnings only mean this script runs without a KiCad
    # user config; the footprints are embedded in the board file.
    kinds = re.findall(r"^\[(\w+)\]:", text, re.M)
    real = [k for k in kinds if k != "lib_footprint_issues"]
    unconnected = int(re.search(r"\*\* Found (\d+) unconnected pads", text).group(1))
    print(f"DRC: {len(real)} violations, {unconnected} unconnected pads")
    if real:
        fail(f"DRC found {len(real)} violations: {sorted(set(real))} (see build/drc.rpt)")
    if unconnected:
        fail(f"DRC found {unconnected} unconnected pads")


def pcb_nets():
    board = pcbnew.LoadBoard(PCB)
    groups = {}
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            if pad.GetNetname():
                groups.setdefault(pad.GetNetname(), set()).add((fp.GetReference(), pad.GetNumber()))
    return groups


def sch_nets():
    out = os.path.join(BUILD, NAME + ".net")
    subprocess.run(["kicad-cli", "sch", "export", "netlist", "--output", out, SCH],
                   check=True, stdout=subprocess.DEVNULL)
    text = open(out).read()
    groups = {}
    for block in text.split("(net (code ")[1:]:
        name = re.search(r'\(name "([^"]+)"\)', block).group(1).lstrip("/")
        groups[name] = set(re.findall(r'\(node \(ref "([^"]+)"\) \(pin "([^"]+)"\)', block))
    return groups


def compare_netlists():
    pcb = pcb_nets()
    sch = {k: v for k, v in sch_nets().items() if not k.startswith("unconnected-")}
    pcb_sets = sorted(sorted(v) for v in pcb.values())
    sch_sets = sorted(sorted(v) for v in sch.values() if len(v) > 0)
    # single-pin nets in the schematic are unconnected pins; the PCB has none
    sch_sets = [s for s in sch_sets if len(s) > 1 or s in pcb_sets]
    if pcb_sets != sch_sets:
        only_pcb = [s for s in pcb_sets if s not in sch_sets]
        only_sch = [s for s in sch_sets if s not in pcb_sets]
        fail(f"schematic and PCB netlists differ: pcb-only {only_pcb[:3]} sch-only {only_sch[:3]}")
    else:
        print(f"Netlist: schematic and PCB match ({len(pcb_sets)} nets)")
    return pcb


def check_firmware(pcb):
    cfg = open(os.path.join(REPO, "configs", "RP2040AdvancedBreakoutBoardUSBPassthroughMatrix", "BoardConfig.h")).read()
    rows = [int(v) for v in re.search(r"#define KEY_MATRIX_ROW_PINS ([\d, ]+)", cfg).group(1).split(",")]
    cols = [int(v) for v in re.search(r"#define KEY_MATRIX_COL_PINS ([\d, ]+)", cfg).group(1).split(",")]
    if rows != generate.ROW_GPIOS or cols != generate.COL_GPIOS:
        fail(f"board config rows {rows} cols {cols} differ from the adapter")
    # every header pin carries the right GPIO, and every switch has its diode
    # from its own terminal to its row, cathode on the row
    header = {pin: net for net, nodes in pcb.items() for ref, pin in nodes if ref == "J1"}
    for pin, sig in generate.HEADER.items():
        want = generate.header_net(pin)
        if header.get(str(pin)) != want:
            fail(f"J1 pin {pin} is {header.get(str(pin))}, expected {want} ({sig})")
    seen = set()
    for net, nodes in pcb.items():
        m = re.match(r"SW_R(\d)C(\d)", net)
        if not m:
            continue
        r, c = int(m.group(1)) - 1, int(m.group(2)) - 1
        diode = [ref for ref, pin in nodes if ref.startswith("D") and pin == "2"]
        term = [ref for ref, pin in nodes if ref == f"J{3 + c}" and pin == str(r + 1)]
        if len(diode) != 1 or len(term) != 1 or len(nodes) != 2:
            fail(f"{net} should join one terminal and one diode anode, has {sorted(nodes)}")
            continue
        cathode_net = [n for n, ns in pcb.items() if (diode[0], "1") in ns]
        if cathode_net != [generate.row_net(r)]:
            fail(f"{diode[0]} cathode is on {cathode_net}, expected {generate.row_net(r)}")
        seen.add((r, c))
    if len(seen) != len(rows) * len(cols):
        fail(f"only {len(seen)} of {len(rows) * len(cols)} switches are wired")
    else:
        print(f"Matrix: {len(rows)} rows x {len(cols)} columns, {len(seen)} switches, one diode each, matches board config")


def fab():
    os.makedirs(FAB, exist_ok=True)
    run = lambda *a: subprocess.run(["kicad-cli", *a], check=True, stdout=subprocess.DEVNULL)
    gerb = os.path.join(FAB, "gerbers")
    os.makedirs(gerb, exist_ok=True)
    run("pcb", "export", "gerbers", "--output", gerb + "/",
        "--layers", "F.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts", PCB)
    run("pcb", "export", "drill", "--output", gerb + "/", PCB)
    run("pcb", "export", "pos", "--output", os.path.join(FAB, NAME + "-pos.csv"),
        "--format", "csv", "--units", "mm", "--side", "both", PCB)
    run("sch", "export", "pdf", "--output", os.path.join(FAB, NAME + "-schematic.pdf"), SCH)
    run("pcb", "export", "svg", "--output", os.path.join(BUILD, NAME + "-front.svg"),
        "--layers", "F.Cu,F.SilkS,F.Mask,Edge.Cuts", "--page-size-mode", "2", "--exclude-drawing-sheet", PCB)
    run("pcb", "export", "svg", "--output", os.path.join(BUILD, NAME + "-back.svg"),
        "--layers", "B.Cu,B.SilkS,Edge.Cuts", "--page-size-mode", "2", "--exclude-drawing-sheet", "--mirror", PCB)

    # BOM grouped by part
    parts = {
        "Device:D": ("1N4148W", "Small-signal switching diode, SOD-123"),
        "Connector_Generic:Conn_02x10_Odd_Even": ("2x10 box header", "2.54 mm shrouded IDC box header, vertical, through-hole"),
        "Connector:Screw_Terminal_01x07": ("7-way terminal", "3.5 mm pitch screw terminal, Phoenix PT 1,5/7-3,5-H or equivalent"),
        "Connector:Screw_Terminal_01x02": ("2-way terminal", "3.5 mm pitch screw terminal, Phoenix PT 1,5/2-3,5-H or equivalent"),
    }
    groups = {}
    for p in generate.build_parts():
        if p["sym"] in parts:
            groups.setdefault((p["sym"], p["fp"]), []).append(p["ref"])
    with open(os.path.join(FAB, NAME + "-bom.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Qty", "References", "Value", "Footprint", "Description"])
        for (sym, fp), refs in groups.items():
            refs.sort(key=lambda r: int(re.sub(r"\D", "", r)))
            w.writerow([len(refs), " ".join(refs), parts[sym][0], fp, parts[sym][1]])
    print("Fab files written to fab/")


def main():
    drc()
    pcb = compare_netlists()
    check_firmware(pcb)
    fab()
    if errors:
        sys.exit(f"{len(errors)} check(s) failed")
    print("All checks passed")


if __name__ == "__main__":
    main()
