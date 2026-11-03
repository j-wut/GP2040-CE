# Key matrix diode adapter

A small board that turns the 20-pin header of the **RP2040 Advanced Breakout Board (USB Passthrough edition)** into
48 matrix switch inputs with full N-key rollover. It goes with the
[RP2040AdvancedBreakoutBoardUSBPassthroughMatrix](../../configs/RP2040AdvancedBreakoutBoardUSBPassthroughMatrix)
firmware config.

- 112 x 72 mm, 2 layers, all through-hole connectors plus 48 SOD-123 diodes.
- **J1** connects to the breakout board's 20-pin header with a straight 20-pin IDC ribbon cable (pin 1 to pin 1).
- **J2** is an expansion connector with the same pinout, wired in parallel with J1.
- **J3-J10** are one 7-way screw terminal per matrix column: R1-R6 (one per switch) and C (the column common).
- **J11** brings out GPIO 19 and GND for one ordinary button (UP in the default mapping).
- One 1N4148W diode per switch, already on the board. No diodes are needed in the wiring.

## Wiring a switch

Each switch gets two wires: one to its own terminal (R1-R6) and one to the C terminal of the same block. The C
terminals can be daisy-chained between the switches of a column, the same way ground is daisy-chained on a normal
harness.

```
block "COLUMN 3"   R4 ─── switch ─── C        = matrix switch row 4, column 3
```

Switch row R*n* in column *m* is switch (row *n*, column *m*) in the web config (Add-ons > Keyboard Matrix).
Buttons wired to ground, as on a normal harness, must not be connected to the matrix terminals.

GPIO 20-22 (A2, L3, R3 on the breakout's 4-pin harness) are not on the 20-pin header and stay ordinary buttons on
the breakout board. Don't wire anything else to the breakout's own K1-K4, P1-P4, S1, S2, A1, L, R, D terminals
while the adapter is connected: those GPIOs are matrix lines now.

## Signals

| J1/J2 pin | Signal | Breakout label | | J1/J2 pin | Signal | Breakout label |
|-----|--------|----|-|-----|--------|----|
| 1   | GP19 direct | U  | | 2   | ROW6 (GP18) | D |
| 3   | ROW5 (GP17) | R  | | 4   | ROW4 (GP16) | L |
| 5   | ROW3 (GP15) | S1 | | 6   | ROW2 (GP14) | A1 |
| 7   | ROW1 (GP13) | S2 | | 8   | COL8 (GP12) | P1 |
| 9   | COL7 (GP11) | P2 | | 10  | COL6 (GP10) | P3 |
| 11  | COL5 (GP9)  | P4 | | 12  | GND | G |
| 13  | COL4 (GP8)  | K1 | | 14  | COL3 (GP7) | K2 |
| 15  | COL2 (GP6)  | K3 | | 16  | COL1 (GP5) | K4 |
| 17  | GND | G | | 18  | GND | G |
| 19  | +5V | VCC | | 20  | +5V | VCC |

The pin numbers follow the breakout board's pinout image (pin 1 next to the "U" label, pins 19 and 20 are +5 V).
Check pin 1 on your board before the first power-up.

### Expansion connector (J2)

J2 carries the same matrix rows, columns, GP19, GND and +5 V. Use it to reach matrix positions from a second board,
for example a remote button panel. Anything on J2 must also wire each switch in series with a diode (stripe toward
the row) between a row and a column. A normal 20-pin button harness that wires every button to ground will not work
on J2. A switch wired on J2 and a switch on the same matrix position here act as one input.

## Files

| File | What it is |
|------|------------|
| `keymatrix-adapter.kicad_sch`, `.kicad_pcb`, `.kicad_pro` | KiCad 7 project |
| `fab/gerbers/` | Gerbers and drill file for ordering the PCB |
| `fab/keymatrix-adapter-bom.csv` | Bill of materials |
| `fab/keymatrix-adapter-pos.csv` | Pick-and-place positions for the diodes |
| `fab/keymatrix-adapter-schematic.pdf` | Schematic |
| `generate.py`, `autoroute.py` | Build the schematic and the routed PCB from one parts and nets table |
| `check.py` | Runs the checks below and writes `fab/` |
| `simulate.py`, `sim-results.txt` | ngspice simulation of the full matrix through this board, and its results |

## Checks

`python3 generate.py && python3 check.py` (needs KiCad 7 with its Python module) regenerates the design and checks:

- KiCad DRC: no clearance or other violations, no unconnected pads.
- The schematic netlist (from `kicad-cli`) connects exactly the same pads as the PCB.
- Every one of the 48 switches has its own terminal and its own diode, cathode on the right row, and the rows and
  columns match `KEY_MATRIX_ROW_PINS` / `KEY_MATRIX_COL_PINS` in the board config.

## Simulation

`python3 simulate.py` runs ngspice on the whole 6 x 8 matrix through this board: RP2040 pull-ups and pins, the ribbon
cable, the copper of every net on this PCB (measured from the board file), the 48 diodes, and the harness wiring
(each switch's own wire plus the column common daisy-chained through six switches). It scans the way the firmware
does and checks every read for several held-switch patterns, including all 48 held. Results:
[sim-results.txt](sim-results.txt).

| Wiring (ribbon / switch wire / column chain) | Press seen at the pin | Column recovery | Result with defaults |
|-----|-----|-----|-----|
| short: 0.15 m / 0.2 m / 1 m | 0.1 us | up to 7.6 us | pass |
| typical arcade stick: 0.3 m / 0.5 m / 3 m | 0.2 us | up to 17 us | pass |
| long cabinet: 1 m / 1 m / 6 m | 0.4 us | up to 34 us | pass |

Wire length adds no meaningful latency: a press shows up at the pin in under half a microsecond even with cabinet
wiring. What wiring does set is how long a released row's columns take to climb back through the RP2040's weak
pull-ups. The firmware waits a fixed **row delay** after each row for that (default 40 us, set in the web config
next to the settle time). With the defaults a full scan of all six rows takes about 276 us, well inside the 1 ms
between USB reports. Shorter wiring can use a smaller row delay; see the table in
[docs/keymatrix-sim/results.txt](../../docs/keymatrix-sim/results.txt).

Not built or tested on hardware yet.
