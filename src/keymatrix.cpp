#include "keymatrix.h"
#include "helper.h"

#include "hardware/gpio.h"
#include "hardware/timer.h"

void KeyMatrix::setup(const KeyMatrixOptions& options) {
    enabled = false;
    rowCount = 0;
    colCount = 0;
    switchCount = 0;
    matrixGpioMask = 0;
    colMask = 0;
    state = 0;

    if (!options.enabled)
        return;

    const auto addPin = [&](int32_t pin, uint8_t* pins, uint8_t& count, uint8_t maxCount) -> void {
        if (!isValidPin(pin) || count >= maxCount || (matrixGpioMask & (Mask_t{1} << pin)))
            return;
        pins[count++] = (uint8_t)pin;
        matrixGpioMask |= Mask_t{1} << pin;
    };
    for (pb_size_t i = 0; i < options.rowPins_count; i++)
        addPin(options.rowPins[i], rowPins, rowCount, KEY_MATRIX_MAX_ROWS);
    for (pb_size_t i = 0; i < options.colPins_count; i++)
        addPin(options.colPins[i], colPins, colCount, KEY_MATRIX_MAX_COLS);

    if (rowCount == 0 || colCount == 0) {
        matrixGpioMask = 0;
        return;
    }

    // Give switches the free pin mask bits: the matrix GPIOs' own bits first,
    // then the bits above the real GPIOs.
    const uint32_t wanted = (uint32_t)rowCount * colCount;
    for (uint32_t pin = 0; pin < NUM_INPUT_PINS && switchCount < wanted; pin++) {
        if (pin >= NUM_BANK0_GPIOS || (matrixGpioMask & (Mask_t{1} << pin)))
            switchPins[switchCount++] = (uint8_t)pin;
    }

    settleMicros = options.has_settleMicros ? options.settleMicros : KEY_MATRIX_SETTLE_MICROS;
    rowDelayMicros = options.has_rowDelayMicros ? options.rowDelayMicros : KEY_MATRIX_ROW_DELAY_MICROS;
    if (settleMicros > KEY_MATRIX_MAX_DELAY_MICROS) settleMicros = KEY_MATRIX_MAX_DELAY_MICROS;
    if (rowDelayMicros > KEY_MATRIX_MAX_DELAY_MICROS) rowDelayMicros = KEY_MATRIX_MAX_DELAY_MICROS;

    // Rows idle as pulled-up inputs. A row is pulled low only while it is scanned.
    for (uint8_t r = 0; r < rowCount; r++) {
        gpio_init(rowPins[r]);
        gpio_set_dir(rowPins[r], GPIO_IN);
        gpio_pull_up(rowPins[r]);
        gpio_put(rowPins[r], 0);
    }
    // Columns are pulled-up inputs. A pressed switch pulls its column low.
    for (uint8_t c = 0; c < colCount; c++) {
        gpio_init(colPins[c]);
        gpio_set_dir(colPins[c], GPIO_IN);
        gpio_pull_up(colPins[c]);
        colMask |= Mask_t{1} << colPins[c];
    }

    enabled = true;
}

void __not_in_flash_func(KeyMatrix::scan)() {
    if (!enabled)
        return;

    Mask_t pressed = 0;
    uint32_t index = 0;
    for (uint8_t r = 0; r < rowCount && index < switchCount; r++) {
        const uint8_t row = rowPins[r];
        gpio_set_dir(row, GPIO_OUT); // output value is preset to 0
        busy_wait_us_32(settleMicros);
        const Mask_t lowCols = ~gpio_get_all64() & colMask;
        gpio_set_dir(row, GPIO_IN);
        // let the columns this row pulled low climb back up before the next row
        busy_wait_us_32(rowDelayMicros);

        for (uint8_t c = 0; c < colCount && index < switchCount; c++, index++) {
            if (lowCols & (Mask_t{1} << colPins[c]))
                pressed |= Mask_t{1} << switchPins[index];
        }
    }
    state = pressed;
}
