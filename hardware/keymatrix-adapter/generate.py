#!/usr/bin/env python3
"""Generate the GP2040-CE key matrix diode adapter (KiCad 7 schematic + PCB).

The adapter plugs into the 20-pin header of the RP2040 Advanced Breakout Board
(USB Passthrough edition) with a straight 20-pin IDC ribbon cable. It turns the
14 button GPIOs on that header into a 6 x 8 diode matrix with one screw terminal
and one diode per switch, matching the RP2040AdvancedBreakoutBoardUSBPassthroughMatrix
board config.

Usage (needs KiCad 7 with its Python module and libraries installed):
    python3 generate.py     # write schematic and routed PCB (autoroute.py)
    python3 check.py        # DRC, schematic vs PCB netlist, manufacturing files

Everything (parts, nets, placement) comes from the tables below, so the
schematic and the PCB cannot drift apart. check.py compares them afterwards.
"""

import argparse
import os
import re
import subprocess
import sys
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = "keymatrix-adapter"
KICAD_SYM = "/usr/share/kicad/symbols"
KICAD_FP = "/usr/share/kicad/footprints"

# ---------------------------------------------------------------------------
# Electrical design
# ---------------------------------------------------------------------------

COL_GPIOS = [5, 6, 7, 8, 9, 10, 11, 12]  # must match KEY_MATRIX_COL_PINS
ROW_GPIOS = [13, 14, 15, 16, 17, 18]     # must match KEY_MATRIX_ROW_PINS
DIRECT_GPIO = 19                          # left as a normal button (UP)

# Breakout board 20-pin header, pin number -> signal (see the board's pinout
# image: pin 1 is next to "U", pins 19/20 are +5 V).
HEADER = {
    1: "GP19", 2: "GP18", 3: "GP17", 4: "GP16", 5: "GP15", 6: "GP14",
    7: "GP13", 8: "GP12", 9: "GP11", 10: "GP10", 11: "GP9", 12: "GND",
    13: "GP8", 14: "GP7", 15: "GP6", 16: "GP5", 17: "GND", 18: "GND",
    19: "+5V", 20: "+5V",
}

# Silkscreen names the breakout board prints for each GPIO terminal.
BREAKOUT_LABEL = {5: "K4", 6: "K3", 7: "K2", 8: "K1", 9: "P4", 10: "P3", 11: "P2",
                  12: "P1", 13: "S2", 14: "A1", 15: "S1", 16: "L", 17: "R", 18: "D", 19: "U"}


def col_net(c):
    return f"COL{c + 1}"


def row_net(r):
    return f"ROW{r + 1}"


def sw_net(r, c):
    return f"SW_R{r + 1}C{c + 1}"


def gpio_net(gpio):
    if gpio in COL_GPIOS:
        return col_net(COL_GPIOS.index(gpio))
    if gpio in ROW_GPIOS:
        return row_net(ROW_GPIOS.index(gpio))
    return f"GP{gpio}"


def header_net(pin):
    sig = HEADER[pin]
    return gpio_net(int(sig[2:])) if sig.startswith("GP") else sig


FP_HEADER = "Connector_IDC:IDC-Header_2x10_P2.54mm_Vertical"
FP_TERM7 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_PT-1,5-7-3.5-H_1x07_P3.50mm_Horizontal"
FP_TERM2 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_PT-1,5-2-3.5-H_1x02_P3.50mm_Horizontal"
FP_DIODE = "Diode_SMD:D_SOD-123"
FP_HOLE = "MountingHole:MountingHole_3.2mm_M3"


def build_parts():
    """List of parts: ref, lib symbol, value, footprint, {pin: net}."""
    parts = []
    hdr = {str(p): header_net(p) for p in HEADER}
    parts.append(dict(ref="J1", sym="Connector_Generic:Conn_02x10_Odd_Even",
                      value="To breakout", fp=FP_HEADER, pins=hdr))
    parts.append(dict(ref="J2", sym="Connector_Generic:Conn_02x10_Odd_Even",
                      value="Expansion", fp=FP_HEADER, pins=dict(hdr)))
    for c in range(len(COL_GPIOS)):
        pins = {str(r + 1): sw_net(r, c) for r in range(len(ROW_GPIOS))}
        pins["7"] = col_net(c)
        parts.append(dict(ref=f"J{3 + c}", sym="Connector:Screw_Terminal_01x07",
                          value=f"Column {c + 1}", fp=FP_TERM7, pins=pins))
    parts.append(dict(ref="J11", sym="Connector:Screw_Terminal_01x02",
                      value=f"GP{DIRECT_GPIO} direct", fp=FP_TERM2,
                      pins={"1": gpio_net(DIRECT_GPIO), "2": "GND"}))
    n = 1
    for c in range(len(COL_GPIOS)):
        for r in range(len(ROW_GPIOS)):
            # pin 1 = K (cathode, stripe) to the row, pin 2 = A to the switch
            parts.append(dict(ref=f"D{n}", sym="Device:D", value="1N4148W", fp=FP_DIODE,
                              pins={"1": row_net(r), "2": sw_net(r, c)}, row=r, col=c))
            n += 1
    for i in range(4):
        parts.append(dict(ref=f"H{i + 1}", sym="Mechanical:MountingHole", value="M3",
                          fp=FP_HOLE, pins={}))
    return parts


# ---------------------------------------------------------------------------
# Schematic
# ---------------------------------------------------------------------------

def uid():
    return str(uuid.uuid4())


def extract_symbol(lib, name):
    text = open(os.path.join(KICAD_SYM, lib + ".kicad_sym")).read()
    start = text.index(f'\n  (symbol "{name}"') + 1
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                body = text[start:i + 1]
                break
    if "(extends" in body:
        raise RuntimeError(f"{lib}:{name} extends another symbol")
    pins = {}
    for m in re.finditer(r'\(pin \w+ \w+ \(at ([-\d.]+) ([-\d.]+) ([-\d.]+)\).*?\(number "([^"]+)"', body, re.S):
        pins[m.group(4)] = (float(m.group(1)), float(m.group(2)), float(m.group(3)))
    body = body.replace(f'(symbol "{name}"', f'(symbol "{lib}:{name}"', 1)
    return body, pins


def sch_pos(x, y, px, py):
    # symbol library is y-up, schematic is y-down (all symbols placed unrotated)
    return round(x + px, 2), round(y - py, 2)


def snap(v):
    return round(round(v / 1.27) * 1.27, 2)


def build_schematic(parts):
    root = uid()
    libs = {}
    for p in parts:
        lib, name = p["sym"].split(":")
        if p["sym"] not in libs:
            libs[p["sym"]] = extract_symbol(lib, name)

    out = []
    items = []

    def text(s, x, y, size=1.27):
        items.append(f'  (text "{s}" (at {x} {y} 0)\n    (effects (font (size {size} {size})) (justify left bottom))\n    (uuid {uid()})\n  )')

    def wire(x1, y1, x2, y2):
        items.append(f'  (wire (pts (xy {x1} {y1}) (xy {x2} {y2}))\n    (stroke (width 0) (type default))\n    (uuid {uid()})\n  )')

    def label(net, x, y, angle):
        just = "right bottom" if angle == 180 else "left bottom"
        items.append(f'  (label "{net}" (at {x} {y} {angle}) (fields_autoplaced)\n    (effects (font (size 1.27 1.27)) (justify {just}))\n    (uuid {uid()})\n  )')

    def power(net, x, y, angle):
        items.append(f'  (global_label "{net}" (shape passive) (at {x} {y} {angle}) (fields_autoplaced)\n    (effects (font (size 1.27 1.27)) (justify {"right" if angle == 180 else "left"}))\n    (uuid {uid()})\n  )')

    def place(p, x, y, ref_at=None, val_at=None):
        x, y = snap(x), snap(y)
        _, pins = libs[p["sym"]]
        rx, ry = ref_at or (x, y - 2.54)
        vx, vy = val_at or (x, y + 2.54)
        pin_uuids = "".join(f'\n    (pin "{n}" (uuid {uid()}))' for n in pins)
        items.append(
            f'  (symbol (lib_id "{p["sym"]}") (at {x} {y} 0) (unit 1)\n'
            f'    (in_bom yes) (on_board yes) (dnp no)\n'
            f'    (uuid {uid()})\n'
            f'    (property "Reference" "{p["ref"]}" (at {rx} {ry} 0)\n      (effects (font (size 1.27 1.27)))\n    )\n'
            f'    (property "Value" "{p["value"]}" (at {vx} {vy} 0)\n      (effects (font (size 1.27 1.27)))\n    )\n'
            f'    (property "Footprint" "{p["fp"]}" (at {x} {y} 0)\n      (effects (font (size 1.27 1.27)) hide)\n    )\n'
            f'    (property "Datasheet" "~" (at {x} {y} 0)\n      (effects (font (size 1.27 1.27)) hide)\n    )'
            f'{pin_uuids}\n'
            f'    (instances\n      (project "{NAME}"\n        (path "/{root}"\n          (reference "{p["ref"]}") (unit 1)\n        )\n      )\n    )\n  )')
        # wire stub + net label on every connected pin
        for n, net in p["pins"].items():
            px, py, pa = pins[n]
            ex, ey = sch_pos(x, y, px, py)
            # pin points away from the body: angle 0 = pin on the left side
            dx = -2.54 if pa == 0 else 2.54
            if pa in (90, 270):
                dx = 0
            lx = round(ex + dx, 2)
            wire(ex, ey, lx, ey)
            if net in ("GND", "+5V"):
                power(net, lx, ey, 180 if dx < 0 else 0)
            else:
                label(net, lx, ey, 180 if dx < 0 else 0)

    byref = {p["ref"]: p for p in parts}
    # connectors on the left
    text("Breakout board 20-pin header (straight IDC ribbon, pin 1 to pin 1)", 20.32, 25.4)
    hdr_txt = lambda y: dict(ref_at=(39.37, snap(y) - 15.24), val_at=(39.37, snap(y) + 15.24))
    place(byref["J1"], 38.1, 45.72, **hdr_txt(45.72))
    text("Expansion: same pinout, wired in parallel with J1", 20.32, 81.28)
    place(byref["J2"], 38.1, 101.6, **hdr_txt(101.6))
    text(f"GPIO {DIRECT_GPIO} stays a normal button (UP), wired to ground", 20.32, 132.08)
    place(byref["J11"], 38.1, 142.24, ref_at=(38.1, 137.16), val_at=(38.1, 149.86))
    for i in range(4):
        place(byref[f"H{i + 1}"], 25.4 + i * 12.7, 165.1)

    text("Switch matrix: one screw terminal and one diode per switch. Wire each switch between", 88.9, 22.86)
    text("its terminal (1-6) and the column's COM terminal. Diode stripe (K) faces the row.", 88.9, 26.67)
    text("Rows: GP13-GP18 (breakout S2 A1 S1 L R D). Columns: GP5-GP12 (K4 K3 K2 K1 P4 P3 P2 P1).", 88.9, 30.48)
    for c in range(len(COL_GPIOS)):
        gx = 88.9 + (c % 4) * 76.2
        gy = 45.72 + (c // 4) * 66.04
        g = COL_GPIOS[c]
        text(f"Column {c + 1}: GP{g} ({BREAKOUT_LABEL[g]})", gx, gy - 7.62)
        place(byref[f"J{3 + c}"], gx + 7.62, gy + 7.62,
              ref_at=(snap(gx + 7.62), snap(gy + 7.62) - 10.16), val_at=(snap(gx + 7.62), snap(gy + 7.62) + 12.7))
        for r in range(len(ROW_GPIOS)):
            d = next(p for p in parts if p.get("col") == c and p.get("row") == r)
            dy = gy + r * 5.08
            place(d, gx + 43.18, dy, ref_at=(snap(gx + 43.18), snap(dy) - 1.9), val_at=(snap(gx + 43.18) + 12, snap(dy) + 30))
    # hide diode values (they all say 1N4148W) by moving them off the grid area is ugly;
    # instead state it once
    text("D1-D48: 1N4148W (SOD-123), cathode/stripe to ROW", 88.9, 180.34)

    out.append('(kicad_sch (version 20230121) (generator eeschema)\n')
    out.append(f'  (uuid {root})\n')
    out.append('  (paper "A3")\n')
    out.append(f'  (title_block\n    (title "GP2040-CE Key Matrix Diode Adapter")\n    (rev "1.0")\n'
               f'    (comment 1 "For RP2040 Advanced Breakout Board - USB Passthrough, 20-pin header")\n'
               f'    (comment 2 "Firmware: RP2040AdvancedBreakoutBoardUSBPassthroughMatrix")\n  )\n')
    out.append('  (lib_symbols\n')
    for body, _ in libs.values():
        out.append("    " + body.replace("\n", "\n  ") + "\n")
    out.append('  )\n')
    out.append("\n".join(items) + "\n")
    out.append(f'  (sheet_instances\n    (path "/" (page "1"))\n  )\n)\n')
    # value text of diodes would overlap; hide them
    sch = "".join(out)
    sch = re.sub(r'(\(property "Value" "1N4148W" \(at [^)]*\)\n      \(effects \(font \(size 1.27 1.27\)\))\)',
                 r'\1 hide)', sch)
    with open(os.path.join(HERE, NAME + ".kicad_sch"), "w") as f:
        f.write(sch)


# ---------------------------------------------------------------------------
# PCB
# ---------------------------------------------------------------------------

BOARD_W = 112.0
BOARD_H = 72.0
TERM_Y = 5.6          # pad row of the terminal blocks, from the top/bottom edge
BLOCK_PITCH = 26.6    # one 7-way block per column, four along each long edge
BLOCK_X0 = 5.6        # pad 1 of the first block (left edge of its body ~3.3)
DIODE_DY = 9.0        # diode centre, measured inward from the terminal pads


def term_pad_x(c, r):
    """x of terminal pin (r = 0..5 switch, 6 = COM) for column c."""
    k = c % 4
    return BLOCK_X0 + k * BLOCK_PITCH + r * 3.5


def build_pcb(parts):
    import pcbnew

    board = pcbnew.BOARD()
    mm = pcbnew.FromMM

    def vec(x, y):
        return pcbnew.VECTOR2I(mm(x), mm(y))

    nets = {}

    def net(name):
        if name not in nets:
            ni = pcbnew.NETINFO_ITEM(board, name)
            board.Add(ni)
            nets[name] = ni
        return nets[name]

    # outline
    pts = [(0, 0), (BOARD_W, 0), (BOARD_W, BOARD_H), (0, BOARD_H)]
    for i in range(4):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(vec(*pts[i]))
        seg.SetEnd(vec(*pts[(i + 1) % 4]))
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetWidth(mm(0.1))
        board.Add(seg)

    ds = board.GetDesignSettings()
    ds.m_TrackMinWidth = mm(0.2)
    ds.m_MinClearance = mm(0.2)
    ds.m_ViasMinSize = mm(0.6)
    ds.m_MinThroughDrill = mm(0.3)
    ds.m_CopperEdgeClearance = mm(0.3)
    nc = ds.m_NetSettings.m_DefaultNetClass
    nc.SetTrackWidth(mm(0.3))
    nc.SetClearance(mm(0.2))
    nc.SetViaDiameter(mm(0.7))
    nc.SetViaDrill(mm(0.35))

    def add(p, x, y, angle):
        lib, name = p["fp"].split(":")
        fp = pcbnew.FootprintLoad(os.path.join(KICAD_FP, lib + ".pretty"), name)
        fp.SetReference(p["ref"])
        fp.SetValue(p["value"])
        fp.SetFPIDAsString(p["fp"])
        board.Add(fp)
        fp.SetOrientationDegrees(angle)
        fp.SetPosition(vec(x, y))
        for pad in fp.Pads():
            n = pad.GetNumber()
            if n in p["pins"]:
                pad.SetNet(net(p["pins"][n]))
        return fp

    byref = {p["ref"]: p for p in parts}
    fps = {}

    # Terminal blocks: columns 1-4 along the top edge, 5-8 along the bottom,
    # wire entry facing the board edge. Pin 1 (row 1) on the left.
    for c in range(len(COL_GPIOS)):
        top = c < 4
        x1 = term_pad_x(c, 0)
        if top:  # rotated 180: pad 1 ends up on the right, so anchor at the right end
            fps[f"J{3 + c}"] = add(byref[f"J{3 + c}"], x1 + 21.0, TERM_Y, 180)
        else:
            fps[f"J{3 + c}"] = add(byref[f"J{3 + c}"], x1, BOARD_H - TERM_Y, 0)

    # Diodes sit right behind their terminal pin, anode toward the terminal.
    for p in parts:
        if p["ref"].startswith("D"):
            c, r = p["col"], p["row"]
            top = c < 4
            rr = r if not top else 6 - r  # top blocks are rotated, pins run right-to-left
            x = term_pad_x(c, rr)
            if top:
                fps[p["ref"]] = add(p, x, TERM_Y + DIODE_DY, 90)            # pad 2 (A) up
            else:
                fps[p["ref"]] = add(p, x, BOARD_H - TERM_Y - DIODE_DY, 270)  # pad 2 (A) down

    # Headers: horizontal, side by side in the middle. Pin 1 at the left of the
    # lower row so the keying notch faces down.
    hy = BOARD_H / 2
    fps["J1"] = add(byref["J1"], 18.0, hy + 1.27, 90)
    fps["J2"] = add(byref["J2"], 64.0, hy + 1.27, 90)
    fps["J11"] = add(byref["J11"], BOARD_W - 5.6, hy + 1.75, 90)

    for i, (x, y) in enumerate([(5.0, 24.0), (5.0, BOARD_H - 24.0),
                                (BOARD_W - 15.0, 24.0), (BOARD_W - 15.0, BOARD_H - 24.0)]):
        fps[f"H{i + 1}"] = add(byref[f"H{i + 1}"], x, y, 0)

    # Reference designators stay on the fab layer; the silkscreen gets plain
    # labels instead (column and row names, connector names).
    for fp in fps.values():
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)

    def silk(s, x, y, size=1.0, layer=None, angle=0, bold=False):
        t = pcbnew.PCB_TEXT(board)
        t.SetText(s)
        t.SetPosition(vec(x, y))
        t.SetLayer(layer if layer is not None else pcbnew.F_SilkS)
        t.SetTextSize(vec(size, size))
        t.SetTextThickness(mm(size * (0.2 if bold else 0.15)))
        t.SetTextAngleDegrees(angle)
        board.Add(t)

    for c in range(len(COL_GPIOS)):
        top = c < 4
        ly = TERM_Y + 4.4 if top else BOARD_H - TERM_Y - 4.4
        for r in range(7):
            rr = r if not top else 6 - r
            name = f"R{r + 1}" if r < 6 else "C"
            silk(name, term_pad_x(c, rr), ly, 0.9)
        g = COL_GPIOS[c]
        hx = term_pad_x(c, 0) + 10.5
        hy2 = TERM_Y + 13.6 if top else BOARD_H - TERM_Y - 13.6
        silk(f"COLUMN {c + 1} (GP{g} {BREAKOUT_LABEL[g]})", hx, hy2, 1.0)

    silk("J1: TO BREAKOUT 20-PIN", 18.0 + 11.43, hy - 5.6, 1.0, bold=True)
    silk("J2: EXPANSION (SAME PINOUT)", 64.0 + 11.43, hy - 5.6, 1.0)
    silk(f"GP{DIRECT_GPIO} (UP)", BOARD_W - 13.5, hy + 1.75, 0.9)
    silk("GND", BOARD_W - 13.5, hy - 1.75, 0.9)
    silk("GP2040-CE KEY MATRIX ADAPTER v1.0", BOARD_W / 2, hy + 8.0, 1.2, bold=True)
    silk("SWITCH: R1-R6 TERMINAL <-> SAME BLOCK C   DIODE STRIPE TO ROW", BOARD_W / 2, hy + 10.0, 0.9)

    path = os.path.join(HERE, NAME + ".kicad_pcb")
    board.SetFileName(path)
    pcbnew.SaveBoard(path, board)
    return path


def route():
    import pcbnew
    import autoroute

    path = os.path.join(HERE, NAME + ".kicad_pcb")
    board = pcbnew.LoadBoard(path)
    r = autoroute.Router(board, BOARD_W, BOARD_H)
    # group pads by net
    by_net = {}
    for key, (code, cells, centre, layers) in r.pad_cells.items():
        by_net.setdefault(code, []).append((layers, cells, centre, key))
    netinfo = board.GetNetInfo()
    order = []
    for code in by_net:
        n = netinfo.GetNetItem(code).GetNetname()
        rank = 0 if n.startswith("SW_") else 1 if n.startswith("ROW") else 2 if n.startswith("COL") else 3
        order.append((rank, len(by_net[code]), n, code))
    order.sort()
    failed = []
    for rank, count, n, code in order:
        terms = sorted(by_net[code], key=lambda t: (t[2][0], t[2][1]))
        if not r.route_net(code, [(t[0], t[1], t[2]) for t in terms]):
            failed.append(n)
    r.write()

    # ground pour on both layers
    gnd = board.FindNet("GND")
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        z = pcbnew.ZONE(board)
        z.SetLayer(layer)
        z.SetNet(gnd)
        z.SetLocalClearance(pcbnew.FromMM(0.3))
        z.SetMinThickness(pcbnew.FromMM(0.25))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        ol = z.Outline()
        ol.NewOutline()
        for x, y in [(0.5, 0.5), (BOARD_W - 0.5, 0.5), (BOARD_W - 0.5, BOARD_H - 0.5), (0.5, BOARD_H - 0.5)]:
            ol.Append(pcbnew.FromMM(x), pcbnew.FromMM(y))
        board.Add(z)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(path, board)
    if failed:
        sys.exit("could not route: " + ", ".join(failed))
    print(f"routed {len(order)} nets, {len(r.via_list)} vias")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-route", action="store_true", help="leave the PCB unrouted")
    args = ap.parse_args()
    parts = build_parts()
    build_schematic(parts)
    build_pcb(parts)
    if not args.no_route:
        route()


if __name__ == "__main__":
    main()
