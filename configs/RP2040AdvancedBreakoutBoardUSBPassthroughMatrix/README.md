# GP2040 Configuration for the RP2040 Advanced Breakout Board USB Passthrough Edition (Matrix)

Same board as [RP2040AdvancedBreakoutBoardUSBPassthrough](../RP2040AdvancedBreakoutBoardUSBPassthrough), but the 14 button
pins on the 20-pin header are wired as a keyboard-style switch matrix with 48 switches. USB passthrough, RGB LEDs,
the I2C display and the toggle/option connectors are unchanged.

## Wiring

The easiest way is the [key matrix diode adapter](../../hardware/keymatrix-adapter): it plugs into the 20-pin header
with a ribbon cable and has a screw terminal and a diode for every switch.

To wire it by hand, connect each switch between one **row** and one **column**, not to ground. For full N-key
rollover (any number of switches held at once, with no ghost presses) every switch needs its own diode, for example
a 1N4148, in series with the switch, with the stripe (cathode) toward the row wire.

```
column terminal ──── switch ────►|──── row terminal
                               (stripe toward row)
```

| Role    | GPIO | Terminal | 20-pin header pin |
|---------|------|----------|----|
| Column 1 | 5  | K4 | 16 |
| Column 2 | 6  | K3 | 15 |
| Column 3 | 7  | K2 | 14 |
| Column 4 | 8  | K1 | 13 |
| Column 5 | 9  | P4 | 11 |
| Column 6 | 10 | P3 | 10 |
| Column 7 | 11 | P2 | 9 |
| Column 8 | 12 | P1 | 8 |
| Row 1 | 13 | S2 | 7 |
| Row 2 | 14 | A1 | 6 |
| Row 3 | 15 | S1 | 5 |
| Row 4 | 16 | L  | 4 |
| Row 5 | 17 | R  | 3 |
| Row 6 | 18 | D  | 2 |

UP (GPIO 19), A2 (GPIO 20), L3 (GPIO 21) and R3 (GPIO 22) stay ordinary buttons wired to ground.

## Default switch mapping

Every matrix switch is an input pin and is remapped in the web config (Add-ons > Keyboard Matrix) like any other pin.
Rows 1 and 2 reuse the pin numbers of the GPIOs they replace with their standard buttons, so LED and display layouts
keep working.

| Row | C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 |
|-----|----|----|----|----|----|----|----|----|
| 1 (pins 5-12) | L2 | R2 | B2 | B1 | L1 | R1 | B4 | B3 |
| 2 (pins 13-18, 30, 31) | S2 | A1 | S1 | Left | Right | Down | A3 | A4 |
| 3 (pins 32-39) | E1 | E2 | E3 | E4 | E5 | E6 | E7 | E8 |
| 4 (pins 40-47) | E9 | E10 | E11 | E12 | Fn | - | - | - |
| 5 (pins 48-55) | - | - | - | - | - | - | - | - |
| 6 (pins 56-63) | - | - | - | - | - | - | - | - |

## Limits

- The firmware tracks inputs in a 64-bit pin mask. Matrix switches use the bits of the row/column GPIOs plus the bits
  above the real GPIOs (30-63), so a matrix on N pins gets at most N + 34 switches.
- Switch mappings are shared by all profiles.
- Boot-mode button combos only read real GPIO buttons.
