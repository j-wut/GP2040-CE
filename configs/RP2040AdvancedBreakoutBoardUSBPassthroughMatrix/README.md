# GP2040 Configuration for the RP2040 Advanced Breakout Board USB Passthrough Edition (Matrix)

Same board as [RP2040AdvancedBreakoutBoardUSBPassthrough](../RP2040AdvancedBreakoutBoardUSBPassthrough), but all 15 button
pins on the 20-pin header are wired as a keyboard-style switch matrix: 4 rows x 11 columns, 44 switches. USB passthrough, RGB LEDs,
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
| Column 1 | 5 | K4 | 16 |
| Column 2 | 6 | K3 | 15 |
| Column 3 | 7 | K2 | 14 |
| Column 4 | 8 | K1 | 13 |
| Column 5 | 9 | P4 | 11 |
| Column 6 | 10 | P3 | 10 |
| Column 7 | 11 | P2 | 9 |
| Column 8 | 12 | P1 | 8 |
| Column 9 | 13 | S2 | 7 |
| Column 10 | 14 | A1 | 6 |
| Column 11 | 15 | S1 | 5 |
| Row 1 | 16 | L | 4 |
| Row 2 | 17 | R | 3 |
| Row 3 | 18 | D | 2 |
| Row 4 | 19 | U | 1 |

Only 4 rows are scanned, so a full scan takes about 4 x (settle + row delay + 1 us), about 224 us with the defaults.

A2 (GPIO 20), L3 (GPIO 21) and R3 (GPIO 22) stay ordinary buttons wired to ground.

## Default switch mapping

Every matrix switch is an input pin and is remapped in the web config (Add-ons > Keyboard Matrix) like any other pin.
The first 15 switches reuse the pin numbers of GPIO 5-19 with their standard buttons, so every original button is
still there and LED and display layouts keep working.

| Row | C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 | C9 | C10 | C11 |
|-----|----|----|----|----|----|----|----|----|----|-----|-----|
| 1 (pins 5-15) | L2 | R2 | B2 | B1 | L1 | R1 | B4 | B3 | S2 | A1 | S1 |
| 2 (pins 16-19, 30-36) | Left | Right | Down | Up | A3 | A4 | E1 | E2 | E3 | E4 | E5 |
| 3 (pins 37-47) | E6 | E7 | E8 | E9 | E10 | E11 | E12 | Fn | - | - | - |
| 4 (pins 48-58) | - | - | - | - | - | - | - | - | - | - | - |

## Profiles, boot modes and LEDs

Matrix switches are input pins everywhere in the firmware:

- **Profiles:** each profile can map the switches its own way (Add-ons > Keyboard Matrix, pick the profile). A
  profile without its own switch mappings uses profile 1's.
- **Boot modes:** any switch can be part of a boot-mode button combo (Boot Input Modes page).
- **LEDs:** a light can be tied to any switch, and every switch has its own static colors per LED profile.

The saved settings area grows from 32 KB to 40 KB for this. It is at the end of flash, so settings saved by older
firmware are kept on upgrade.

## Limits

- The firmware tracks inputs in a 64-bit pin mask. Matrix switches use the bits of the row/column GPIOs plus the bits
  above the real GPIOs (30-63), so a matrix on N pins gets at most N + 34 switches.
