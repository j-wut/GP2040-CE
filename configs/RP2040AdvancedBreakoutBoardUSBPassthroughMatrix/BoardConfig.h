/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef PICO_BOARD_CONFIG_H_
#define PICO_BOARD_CONFIG_H_

#include "enums.pb.h"
#include "class/hid/hid.h"

#define BOARD_CONFIG_LABEL "RP2040 Advanced Breakout Board - USB Passthrough (Matrix)"

// Keyboard-style switch matrix
//
// All 15 button pins on the 20-pin header are wired as a 4 x 11 switch matrix
// (44 switches). Only 4 rows are scanned, so a full scan is short. The matching
// diode adapter board (hardware/keymatrix-adapter) plugs into the 20-pin
// header and has a diode per switch. Without the adapter, wire every switch in
// series with its own diode (e.g. 1N4148), stripe toward the ROW wire, between
// a row and a column.
//
//   Columns (11): GPIO 5  6  7  8  9  10 11 12 13 14 15  (K4 K3 K2 K1 P4 P3 P2 P1 S2 A1 S1)
//   Rows     (4): GPIO 16 17 18 19                         (L R D U)
//
// A2 (GPIO 20), L3 (21) and R3 (22) stay ordinary buttons wired to ground.
//
// Every switch is an input pin that is mapped like any other pin. The first 15
// switches reuse the pin numbers of the GPIOs they replace (5-19) with the
// standard actions, so LED and display layouts keep working. The rest get
// input pins 30 and up.
#define KEY_MATRIX_ENABLED 1
#define KEY_MATRIX_COL_PINS 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
#define KEY_MATRIX_ROW_PINS 16, 17, 18, 19
#define KEY_MATRIX_PIN_ACTIONS \
    /* row 1 (GPIO 16), pins 5-15 */ \
    GpioAction::BUTTON_PRESS_L2, GpioAction::BUTTON_PRESS_R2, GpioAction::BUTTON_PRESS_B2, GpioAction::BUTTON_PRESS_B1, \
    GpioAction::BUTTON_PRESS_L1, GpioAction::BUTTON_PRESS_R1, GpioAction::BUTTON_PRESS_B4, GpioAction::BUTTON_PRESS_B3, \
    GpioAction::BUTTON_PRESS_S2, GpioAction::BUTTON_PRESS_A1, GpioAction::BUTTON_PRESS_S1, \
    /* row 2 (GPIO 17), pins 16-19, 30-36 */ \
    GpioAction::BUTTON_PRESS_LEFT, GpioAction::BUTTON_PRESS_RIGHT, GpioAction::BUTTON_PRESS_DOWN, GpioAction::BUTTON_PRESS_UP, \
    GpioAction::BUTTON_PRESS_A3, GpioAction::BUTTON_PRESS_A4, GpioAction::BUTTON_PRESS_E1, GpioAction::BUTTON_PRESS_E2, GpioAction::BUTTON_PRESS_E3, GpioAction::BUTTON_PRESS_E4, GpioAction::BUTTON_PRESS_E5, \
    /* row 3 (GPIO 18), pins 37-47 */ \
    GpioAction::BUTTON_PRESS_E6, GpioAction::BUTTON_PRESS_E7, GpioAction::BUTTON_PRESS_E8, GpioAction::BUTTON_PRESS_E9, GpioAction::BUTTON_PRESS_E10, GpioAction::BUTTON_PRESS_E11, GpioAction::BUTTON_PRESS_E12, \
    GpioAction::BUTTON_PRESS_FN, GpioAction::NONE, GpioAction::NONE, GpioAction::NONE
    /* row 4 (GPIO 19), pins 48-58: unassigned */

// Direct (non-matrix) buttons
//                                                  // GP2040 | Xinput | Switch  | PS3/4/5  | Dinput | Arcade |
#define GPIO_PIN_20 GpioAction::BUTTON_PRESS_A2     // A2     | ~      | Capture | ~        | 14     | ~      |
#define GPIO_PIN_21 GpioAction::BUTTON_PRESS_L3     // L3     | LS     | LS      | L3       | 11     | LS     |
#define GPIO_PIN_22 GpioAction::BUTTON_PRESS_R3     // R3     | RS     | RS      | R3       | 12     | RS     |

// Setting GPIO pins to assigned by add-on
//
#define GPIO_PIN_00 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_01 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_04 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_23 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_24 GpioAction::ASSIGNED_TO_ADDON
// matrix rows and columns
#define GPIO_PIN_05 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_06 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_07 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_08 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_09 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_10 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_11 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_12 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_13 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_14 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_15 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_16 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_17 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_18 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_19 GpioAction::ASSIGNED_TO_ADDON

// Keyboard Mapping Configuration
//                                            // GP2040 | Xinput | Switch  | PS3/4/5  | Dinput | Arcade |
#define KEY_DPAD_UP     HID_KEY_ARROW_UP      // UP     | UP     | UP      | UP       | UP     | UP     |
#define KEY_DPAD_DOWN   HID_KEY_ARROW_DOWN    // DOWN   | DOWN   | DOWN    | DOWN     | DOWN   | DOWN   |
#define KEY_DPAD_RIGHT  HID_KEY_ARROW_RIGHT   // RIGHT  | RIGHT  | RIGHT   | RIGHT    | RIGHT  | RIGHT  |
#define KEY_DPAD_LEFT   HID_KEY_ARROW_LEFT    // LEFT   | LEFT   | LEFT    | LEFT     | LEFT   | LEFT   |
#define KEY_BUTTON_B1   HID_KEY_SHIFT_LEFT    // B1     | A      | B       | Cross    | 2      | K1     |
#define KEY_BUTTON_B2   HID_KEY_Z             // B2     | B      | A       | Circle   | 3      | K2     |
#define KEY_BUTTON_R2   HID_KEY_X             // R2     | RT     | ZR      | R2       | 8      | K3     |
#define KEY_BUTTON_L2   HID_KEY_V             // L2     | LT     | ZL      | L2       | 7      | K4     |
#define KEY_BUTTON_B3   HID_KEY_CONTROL_LEFT  // B3     | X      | Y       | Square   | 1      | P1     |
#define KEY_BUTTON_B4   HID_KEY_ALT_LEFT      // B4     | Y      | X       | Triangle | 4      | P2     |
#define KEY_BUTTON_R1   HID_KEY_SPACE         // R1     | RB     | R       | R1       | 6      | P3     |
#define KEY_BUTTON_L1   HID_KEY_C             // L1     | LB     | L       | L1       | 5      | P4     |
#define KEY_BUTTON_S1   HID_KEY_5             // S1     | Back   | Minus   | Select   | 9      | Coin   |
#define KEY_BUTTON_S2   HID_KEY_1             // S2     | Start  | Plus    | Start    | 10     | Start  |
#define KEY_BUTTON_L3   HID_KEY_EQUAL         // L3     | LS     | LS      | L3       | 11     | LS     |
#define KEY_BUTTON_R3   HID_KEY_MINUS         // R3     | RS     | RS      | R3       | 12     | RS     |
#define KEY_BUTTON_A1   HID_KEY_9             // A1     | Guide  | Home    | PS       | 13     | ~      |
#define KEY_BUTTON_A2   HID_KEY_F2            // A2     | ~      | Capture | ~        | 14     | ~      |
#define KEY_BUTTON_FN   -1                    // Hotkey Function                                        |

#define USB_PERIPHERAL_ENABLED 1

#define USB_PERIPHERAL_PIN_DPLUS 23
#define USB_PERIPHERAL_PIN_ORDER 0

#define DEFAULT_INPUT_MODE_R1 INPUT_MODE_XBONE
#define DEFAULT_INPUT_MODE_B4 INPUT_MODE_PS5
#define DEFAULT_PS5AUTHENTICATION_TYPE INPUT_MODE_AUTH_TYPE_USB

#define BOARD_LEDS_PIN 4
#define LED_BRIGHTNESS_MAXIMUM 100
#define LED_FORMAT LED_FORMAT_GRB

#define LIGHT_DATA_NAME_DEFAULT "Standard"
#define LIGHT_DATA_SIZE_DEFAULT 12
#define LIGHT_DATA_DEFAULT \
0,  1, 2,  3, 16, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
1,  1, 4,  3, 18, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
2,  1, 6,  4, 17, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
3,  1, 7,  7, 19, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
4,  1, 8,  3, 12, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
5,  1, 10, 2, 11, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
6,  1, 12, 2, 10, LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
7,  1, 14, 2, 9,  LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
8,  1, 8,  5, 8,  LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
9,  1, 10, 4, 7,  LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
10, 1, 12, 4, 6,  LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO, \
11, 1, 14, 4, 5,  LightType::LightType_ActionButton, LightInputSource::LightInputSource_GPIO

// LED Profile 0
#define LEDS_PROFILE0_ENABLED 1
#define LEDS_PROFILE0_BASE_ANIMATION_INDEX AnimationNonPressedEffects::AnimationNonPressedEffects_EFFECT_RAINBOW_ROTATE
#define LEDS_PROFILE0_PRESSED_ANIMATION_INDEX AnimationPressedEffects::AnimationPressedEffects_PRESSEDEFFECT_STATIC_COLOR
#define LEDS_PROFILE0_STATIC_COLOR_PRESSED ColorIndexWhite

#define HAS_I2C_DISPLAY 1
#define I2C0_ENABLED 1
#define I2C0_PIN_SDA 0
#define I2C0_PIN_SCL 1
#define BUTTON_LAYOUT BUTTON_LAYOUT_STICKLESS
#define BUTTON_LAYOUT_RIGHT BUTTON_LAYOUT_STICKLESSB
#define SPLASH_MODE SPLASH_MODE_STATIC
#define SPLASH_DURATION 3000

// Keyboard Host enabled by default
#define KEYBOARD_HOST_ENABLED 1

#endif