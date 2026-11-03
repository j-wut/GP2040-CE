#ifndef _KEYMATRIX_H_
#define _KEYMATRIX_H_

#include <stdint.h>
#include "pico.h"
#include "types.h"
#include "config.pb.h"
#include "BoardConfig.h"

static_assert(_GpioAction_MAX <= 245, "GpioAction no longer fits the packed matrix switch mapping");

/**
 * Keyboard-style row/column switch matrix.
 *
 * Every switch in the matrix shows up to the rest of the firmware as an input
 * pin, mapped, debounced and used exactly like a real button pin. Input pins
 * are the bits of the 64-bit pin mask (Mask_t). Matrix switches take, in order:
 *   1. the bits of the GPIOs used as matrix rows and columns (those GPIOs are
 *      wires now, not buttons), then
 *   2. the bits above the real GPIOs (NUM_BANK0_GPIOS to 63).
 *
 * For full N-key rollover every switch needs its own diode, with the cathode
 * (stripe) toward the row wire. Rows are pulled low one at a time and the
 * columns are read with pull-ups, so every switch is read on every scan.
 */

// Total input pins the firmware tracks, bounded by the width of Mask_t.
#define NUM_INPUT_PINS 64
// Input pins above the real GPIOs.
#define NUM_VIRTUAL_PINS (NUM_INPUT_PINS - NUM_BANK0_GPIOS)

#define KEY_MATRIX_MAX_ROWS 16
#define KEY_MATRIX_MAX_COLS 16
#define KEY_MATRIX_MAX_SWITCHES NUM_INPUT_PINS

#ifndef KEY_MATRIX_ENABLED
#define KEY_MATRIX_ENABLED 0
#endif

// Board configs list the matrix GPIOs, e.g. #define KEY_MATRIX_ROW_PINS 5, 6, 7
#ifndef KEY_MATRIX_ROW_PINS
#define KEY_MATRIX_ROW_PINS -1
#endif

#ifndef KEY_MATRIX_COL_PINS
#define KEY_MATRIX_COL_PINS -1
#endif

// Default action for each switch, row by row, e.g. GpioAction::BUTTON_PRESS_B1, ...
#ifndef KEY_MATRIX_PIN_ACTIONS
#define KEY_MATRIX_PIN_ACTIONS GpioAction::NONE
#endif

// Time to let the column lines settle after pulling a row low.
#ifndef KEY_MATRIX_SETTLE_MICROS
#define KEY_MATRIX_SETTLE_MICROS 10
#endif

// A matrix switch mapping is packed to fit the saved config:
//   switchActions[n]      bits 0-7  GpioAction + 10 (NONE = -10 packs to 0)
//                         bits 8-15 customDpadMask
//   switchButtonMasks[n]  customButtonMask
static inline uint32_t packSwitchAction(GpioAction action, uint32_t dpadMask) {
    return (uint32_t)(uint8_t)((int32_t)action + 10) | ((dpadMask & 0xFF) << 8);
}

static inline GpioMappingInfo unpackSwitchMapping(uint32_t packedAction, uint32_t buttonMask) {
    GpioMappingInfo info = GpioMappingInfo_init_zero;
    info.action = (GpioAction)((int32_t)(packedAction & 0xFF) - 10);
    info.customDpadMask = (packedAction >> 8) & 0xFF;
    info.customButtonMask = buttonMask;
    // switches are plain inputs; they can't be reserved or handed to an add-on
    if (info.action == GpioAction::RESERVED || info.action == GpioAction::ASSIGNED_TO_ADDON)
        info.action = GpioAction::NONE;
    return info;
}

class KeyMatrix {
public:
    static KeyMatrix& getInstance() {
        static KeyMatrix instance;
        return instance;
    }

    // Configure row/column GPIOs from the stored options.
    void setup(const KeyMatrixOptions& options);

    // Scan every row and update the pressed state. No-op when disabled.
    void scan();

    // Pressed switches, as bits of their input pins.
    Mask_t getState() const { return state; }

    bool isEnabled() const { return enabled; }
    // GPIOs used as matrix rows and columns (as a pin mask).
    Mask_t getMatrixGpioMask() const { return matrixGpioMask; }
    bool ownsGpio(int32_t pin) const {
        return pin >= 0 && pin < NUM_INPUT_PINS && (matrixGpioMask & (Mask_t{1} << pin));
    }

    uint8_t getRowCount() const { return rowCount; }
    uint8_t getColCount() const { return colCount; }
    // Switches that got an input pin (rows * cols, capped by free pin mask bits).
    uint8_t getSwitchCount() const { return switchCount; }
    // Input pin of switch N (row by row), or -1.
    int32_t getSwitchPin(uint32_t index) const {
        return index < switchCount ? switchPins[index] : -1;
    }

private:
    KeyMatrix() {}

    bool enabled = false;
    uint8_t rowCount = 0;
    uint8_t colCount = 0;
    uint8_t switchCount = 0;
    uint32_t settleMicros = KEY_MATRIX_SETTLE_MICROS;
    uint8_t rowPins[KEY_MATRIX_MAX_ROWS] = {};
    uint8_t colPins[KEY_MATRIX_MAX_COLS] = {};
    uint8_t switchPins[KEY_MATRIX_MAX_SWITCHES] = {};
    Mask_t matrixGpioMask = 0;
    Mask_t colMask = 0;
    Mask_t state = 0;
};

#endif
