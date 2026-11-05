# Key matrix co-simulation

Tests the firmware's matrix scan (`src/keymatrix.cpp`, compiled unchanged for
the host) against the circuit model of the Key Matrix Diode Adapter in
[gp2040-hardware](https://github.com/j-wut/gp2040-hardware) (`simulate.py`).

1. `harness.cpp` runs the scan with mocked GPIO and a virtual clock and records
   when each row is pulled low, released, and when the columns are read.
2. ngspice simulates the adapter (RP2040 pins and pull-ups, ribbon, PCB copper,
   diodes, switch wiring) with the rows driven on exactly that schedule, and
   samples every column at every read.
3. The scan runs again with those levels. A voltage between 0.8 V and 2.0 V
   fails the test. The firmware's switch state must equal the held switches on
   every scan.

Cases: every switch alone, ghosting/NKRO patterns (ghost rectangle, whole
row/column, all 44, checkerboard, every other row, random sets), for each
wiring length and both pull-up extremes (50k, 80k), plus a row delay sweep.

```
python3 cosim.py "path/to/gp2040-hardware/Boards/Supported Boards/Key Matrix Diode Adapter"
```

Needs g++, ngspice and KiCad 7's `pcbnew` (for PCB copper capacitance).

## Results (adapter at gp2040-hardware 00ac696)

All 72 patterns pass on every wiring case and pull-up, all scans.
Shortest working row delay at the 80k pull-up: 2 us with no wiring, 10 us on a
bench, 15 us in a typical stick, 30 us for cabinet wiring. The default of 50 us
passes everywhere.
