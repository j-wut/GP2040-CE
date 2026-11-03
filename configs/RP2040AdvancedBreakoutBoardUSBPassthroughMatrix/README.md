# GP2040 Configuration for the RP2040 Advanced Breakout Board USB Passthrough Edition (Matrix)

Same board as [RP2040AdvancedBreakoutBoardUSBPassthrough](../RP2040AdvancedBreakoutBoardUSBPassthrough), but the button
terminals are wired as a keyboard-style switch matrix. 14 button pins give 48 switches. USB passthrough, RGB LEDs,
the I2C display and the toggle/option connectors are unchanged.

## Wiring

Each switch connects one **row** terminal to one **column** terminal, not to ground. For full N-key rollover (any
number of switches held at once, with no ghost presses) every switch needs its own diode, for example a 1N4148.
Put the diode in series with the switch, with the stripe (cathode) toward the row wire.

```
column terminal ──── switch ────►|──── row terminal
                               (stripe toward row)
```

| Role    | GPIO | Terminal |
|---------|------|----------|
| Column 1 | 5  | K4 |
| Column 2 | 6  | K3 |
| Column 3 | 7  | K2 |
| Column 4 | 8  | K1 |
| Column 5 | 9  | P4 |
| Column 6 | 10 | P3 |
| Column 7 | 11 | P2 |
| Column 8 | 12 | P1 |
| Row 1 | 16 | L  |
| Row 2 | 17 | R  |
| Row 3 | 18 | D  |
| Row 4 | 19 | U  |
| Row 5 | 21 | L3 (4-pin harness) |
| Row 6 | 22 | R3 (4-pin harness) |

S1 (GPIO 15), S2 (GPIO 13), A1 (GPIO 14) and A2 (GPIO 20) stay ordinary buttons wired to ground.

## Default switch mapping

Every matrix switch is an input pin and is remapped in the web config (Add-ons > Keyboard Matrix) like any other pin.
Rows 1 and 2 reuse the pin numbers of the GPIOs they replace, so LED and display layouts keep working.

| Row | C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 |
|-----|----|----|----|----|----|----|----|----|
| 1 (pins 5-12) | L2 | R2 | B2 | B1 | L1 | R1 | B4 | B3 |
| 2 (pins 16-19, 21, 22, 30, 31) | Left | Right | Down | Up | L3 | R3 | A3 | A4 |
| 3 (pins 32-39) | E1 | E2 | E3 | E4 | E5 | E6 | E7 | E8 |
| 4 (pins 40-47) | E9 | E10 | E11 | E12 | Fn | - | - | - |
| 5 (pins 48-55) | - | - | - | - | - | - | - | - |
| 6 (pins 56-63) | - | - | - | - | - | - | - | - |

## Limits

- The firmware tracks inputs in a 64-bit pin mask. Matrix switches use the bits of the row/column GPIOs plus the bits
  above the real GPIOs (30-63), so a matrix on N pins gets at most N + 34 switches.
- Switch mappings are shared by all profiles.
- Boot-mode button combos only read real GPIO buttons.
