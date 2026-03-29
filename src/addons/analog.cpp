#include "addons/analog.h"
#include "config.pb.h"
#include "enums.pb.h"
#include "hardware/adc.h"
#include "helper.h"
#include "storagemanager.h"
#include "drivermanager.h"

#include <math.h>

#define ADC_MAX ((1 << 12) - 1) // 4095
#define ADC_PIN_OFFSET 26
#define ANALOG_MAX 1.0f
#define ANALOG_CENTER 0.5f
#define ANALOG_MINIMUM 0.0f

bool AnalogInput::available() {
    return Storage::getInstance().getAddonOptions().analogOptions.enabled;
}

void AnalogInput::setup() {
    const AnalogOptions& analogOptions = Storage::getInstance().getAddonOptions().analogOptions;
    
    // Setup our ADC Pair of Sticks
    adc_pairs[0].x_pin = analogOptions.analogAdc1PinX;
    adc_pairs[0].y_pin = analogOptions.analogAdc1PinY;
    adc_pairs[0].analog_invert = analogOptions.analogAdc1Invert;
    adc_pairs[0].analog_dpad = analogOptions.analogAdc1Mode;
    adc_pairs[0].ema_option = analogOptions.analog_smoothing;
    adc_pairs[0].ema_smoothing = analogOptions.smoothing_factor / 1000.0f;
    adc_pairs[0].error_rate = analogOptions.analog_error / 1000.0f;
    adc_pairs[0].in_deadzone = analogOptions.inner_deadzone / 100.0f;
    adc_pairs[0].out_deadzone = analogOptions.outer_deadzone / 100.0f;
    adc_pairs[0].auto_calibration = analogOptions.auto_calibrate;
    adc_pairs[0].forced_circularity = analogOptions.forced_circularity;
    adc_pairs[0].joystick_center_x = analogOptions.joystick_center_x;
    adc_pairs[0].joystick_center_y = analogOptions.joystick_center_y;
    adc_pairs[0].use_mux = analogOptions.analog_mux_1;
    adc_pairs[0].mux_channel_x = analogOptions.analog_channel_x_1;
    adc_pairs[0].mux_channel_y = analogOptions.analog_channel_y_1;

    adc_pairs[0].linearity = analogOptions.analog_linearity_1;
    adc_pairs[0].linearityMargin = analogOptions.analog_linearity_margin_1;
    adc_pairs[0].angle_snapping = analogOptions.analog_angle_snapping_1;
    adc_pairs[0].snap_direction_count = analogOptions.analog_direction_count_1;
    adc_pairs[0].snap_directions = analogOptions.analog_directions_1;
    adc_pairs[0].angle_offset = analogOptions.analog_rotation_offset_1;


    adc_pairs[1].x_pin = analogOptions.analogAdc2PinX;
    adc_pairs[1].y_pin = analogOptions.analogAdc2PinY;
    adc_pairs[1].analog_invert = analogOptions.analogAdc2Invert;
    adc_pairs[1].analog_dpad = analogOptions.analogAdc2Mode;
    adc_pairs[1].ema_option = analogOptions.analog_smoothing2;
    adc_pairs[1].ema_smoothing = analogOptions.smoothing_factor2 / 1000.0f;
    adc_pairs[1].error_rate = analogOptions.analog_error2 / 1000.0f;
    adc_pairs[1].in_deadzone = analogOptions.inner_deadzone2 / 100.0f;
    adc_pairs[1].out_deadzone = analogOptions.outer_deadzone2 / 100.0f;
    adc_pairs[1].auto_calibration = analogOptions.auto_calibrate2;
    adc_pairs[1].forced_circularity = analogOptions.forced_circularity2;
    adc_pairs[1].joystick_center_x = analogOptions.joystick_center_x2;
    adc_pairs[1].joystick_center_y = analogOptions.joystick_center_y2;
    adc_pairs[1].use_mux = analogOptions.analog_mux_2;
    adc_pairs[1].mux_channel_x = analogOptions.analog_channel_x_2;
    adc_pairs[1].mux_channel_y = analogOptions.analog_channel_y_2;

    adc_pairs[1].linearity = analogOptions.analog_linearity_2;
    adc_pairs[1].linearityMargin = analogOptions.analog_linearity_margin_2;
    adc_pairs[1].angle_snapping = analogOptions.analog_angle_snapping_2;
    adc_pairs[1].snap_direction_count = analogOptions.analog_direction_count_2;
    adc_pairs[1].snap_directions = analogOptions.analog_directions_2;
    adc_pairs[1].angle_offset = analogOptions.analog_rotation_offset_2;

    // Setup defaults and helpers
    for (int i = 0; i < ADC_COUNT; i++) {
        adc_pairs[i].x_pin_adc = adc_pairs[i].x_pin - ADC_PIN_OFFSET;
        adc_pairs[i].y_pin_adc = adc_pairs[i].y_pin - ADC_PIN_OFFSET;
        adc_pairs[i].x_value = ANALOG_CENTER;
        adc_pairs[i].y_value = ANALOG_CENTER;
        adc_pairs[i].xy_magnitude = 0.0f;
        adc_pairs[i].x_magnitude = 0.0f;
        adc_pairs[i].y_magnitude = 0.0f;
        adc_pairs[i].x_ema = 0.0f;
        adc_pairs[i].y_ema = 0.0f;
        adc_pairs[i].xy_radians = 0.0f;
        adc_pairs[i].prev_xy_radians = 0.0f;
    }

    if (adc_pairs[0].use_mux || adc_pairs[1].use_mux) {
        switch(analogOptions.analog_mux_channels) {
            case 4:
                this->selectPins = 2;
                break;
            case 8:
                this->selectPins = 3;
                break;
            case 16:
                this->selectPins = 4;
                break;
            case 1:
            default:
                this->selectPins = 0;
                break;
        }

        selectPinArray[0] = analogOptions.analogSelectPin0;
        selectPinArray[1] = analogOptions.analogSelectPin1;
        selectPinArray[2] = analogOptions.analogSelectPin2;
        selectPinArray[3] = analogOptions.analogSelectPin3;
        for(int i = 0; i < selectPins; i++) {
            if ( selectPinArray[i] != -1 ) {
                gpio_init(selectPinArray[i]);
                gpio_set_dir(selectPinArray[i], GPIO_OUT);
                gpio_put(selectPinArray[i], 0);
            }
        }
    }

    // Intialize and auto center X/Y for each pair
    for (int i = 0; i < ADC_COUNT; i++) {
        if(isValidPin(adc_pairs[i].x_pin)) {
            adc_gpio_init(adc_pairs[i].x_pin);
            if (adc_pairs[i].auto_calibration) {
                if(adc_pairs[i].use_mux) {
                    selectChannel(adc_pairs[i].mux_channel_x);
                }
                adc_select_input(adc_pairs[i].x_pin - ADC_PIN_OFFSET);
                adc_pairs[i].x_center = adc_read();
            } else {
                // if auto calibration is disabled, attempt to use stored manual calibration value
                adc_pairs[i].x_center = adc_pairs[i].joystick_center_x;
            }
        }
        if(isValidPin(adc_pairs[i].y_pin)) {
            adc_gpio_init(adc_pairs[i].y_pin);
            if (adc_pairs[i].auto_calibration) {
                if(adc_pairs[i].use_mux) {
                    selectChannel(adc_pairs[i].mux_channel_y);
                }
                adc_select_input(adc_pairs[i].y_pin - ADC_PIN_OFFSET);
                adc_pairs[i].y_center = adc_read();
            } else {
                // if auto calibration is disabled, attempt to use stored manual calibration value
                adc_pairs[i].y_center = adc_pairs[i].joystick_center_y;
            }
        }
    }
}

void AnalogInput::selectChannel(uint8_t channel) {
    for(int i = 0; i < selectPins; i++) {
        if ( selectPinArray[i] != -1 ) {
            gpio_put(selectPinArray[i], (channel >> i) & 0x01);
        }   
    }
}

void AnalogInput::process() {
    Gamepad * gamepad = Storage::getInstance().GetGamepad();
    
    uint32_t joystickMid = GAMEPAD_JOYSTICK_MID;
    uint32_t joystickMax = GAMEPAD_JOYSTICK_MAX;
    if ( DriverManager::getInstance().getDriver() != nullptr ) {
        joystickMid = DriverManager::getInstance().getDriver()->GetJoystickMidValue();
        joystickMax = joystickMid * 2; // 0x8000 mid must be 0x10000 max, but we reduce by 1 if we're maxed out
    }

    for(int i = 0; i < ADC_COUNT; i++) {
        // Read X-Axis
        if (isValidPin(adc_pairs[i].x_pin)) {
            if(adc_pairs[i].use_mux) {
                selectChannel(adc_pairs[i].mux_channel_x);
            }
            adc_pairs[i].x_value = readPin(i, adc_pairs[i].x_pin_adc, adc_pairs[i].x_center);
            if (adc_pairs[i].analog_invert == InvertMode::INVERT_X || 
                adc_pairs[i].analog_invert == InvertMode::INVERT_XY) {
                adc_pairs[i].x_value = ANALOG_MAX - adc_pairs[i].x_value;
            }
            if (adc_pairs[i].ema_option) {
                adc_pairs[i].x_value = emaCalculation(i, adc_pairs[i].x_value, adc_pairs[i].x_ema);
                adc_pairs[i].x_ema = adc_pairs[i].x_value;
            }
        }
        // Read Y-Axis
        if (isValidPin(adc_pairs[i].y_pin)) {
            if(adc_pairs[i].use_mux) {
                selectChannel(adc_pairs[i].mux_channel_y);
            }
            adc_pairs[i].y_value = readPin(i, adc_pairs[i].y_pin_adc, adc_pairs[i].y_center);
            if (adc_pairs[i].analog_invert == InvertMode::INVERT_Y || 
                adc_pairs[i].analog_invert == InvertMode::INVERT_XY) {
                adc_pairs[i].y_value = ANALOG_MAX - adc_pairs[i].y_value;
            }
            if (adc_pairs[i].ema_option) {
                adc_pairs[i].y_value = emaCalculation(i, adc_pairs[i].y_value, adc_pairs[i].y_ema);
                adc_pairs[i].y_ema = adc_pairs[i].y_value;
            }
        }
        // Look for dead-zones and circularity
        adc_pairs[i].xy_magnitude = magnitudeCalculation(i, adc_pairs[i]);
        setRadianDirection(adc_pairs[i]);

        if (adc_pairs[i].linearity) {
            correctLinearity(adc_pairs[i]);
        }

        if (adc_pairs[i].angle_snapping) {
            snapToDirection(adc_pairs[i]);
        }

        adc_pairs[i].x_magnitude = std::cos(adc_pairs[i].xy_radians) * adc_pairs[i].xy_magnitude;
        adc_pairs[i].y_magnitude = std::sin(adc_pairs[i].xy_radians) * adc_pairs[i].xy_magnitude;

        if (adc_pairs[i].xy_magnitude < adc_pairs[i].in_deadzone) {
            adc_pairs[i].x_value = ANALOG_CENTER;
            adc_pairs[i].y_value = ANALOG_CENTER;
        } else {
            radialDeadzone(i, adc_pairs[i]);
        }

        // If MID is 0x8000, clamp our max to 0xFFFF incase we are at 0x10000. 0x7FFF will max at 0xFFFE
        uint16_t clampedX = (uint16_t)std::min((uint32_t)(joystickMax * std::min(adc_pairs[i].x_value, 1.0f)), (uint32_t)0xFFFF);
        uint16_t clampedY = (uint16_t)std::min((uint32_t)(joystickMax * std::min(adc_pairs[i].y_value, 1.0f)), (uint32_t)0xFFFF);

        if (adc_pairs[i].analog_dpad == DpadMode::DPAD_MODE_LEFT_ANALOG) {
            gamepad->state.lx = clampedX;
            gamepad->state.ly = clampedY;
        } else if (adc_pairs[i].analog_dpad == DpadMode::DPAD_MODE_RIGHT_ANALOG) {
            gamepad->state.rx = clampedX;
            gamepad->state.ry = clampedY;
        }
    }
}

float AnalogInput::readPin(int stick_num, Pin_t pin_adc, uint16_t center) {
    adc_select_input(pin_adc);
    uint16_t adc_value = adc_read();
    // Apply calibration only if auto calibration is enabled or manual calibration has been performed
    // Manual calibration is considered performed if the center value is not 0 (default)
    if (adc_pairs[stick_num].auto_calibration || center != 0) {
        if (adc_value > center) {
            adc_value = map(adc_value, center, ADC_MAX, ADC_MAX / 2, ADC_MAX);
        } else if (adc_value == center) {
            adc_value = ADC_MAX / 2;
        } else {
            adc_value = map(adc_value, 0, center, 0, ADC_MAX / 2);
        }
    }
    return ((float)adc_value) / ADC_MAX;
}

float AnalogInput::emaCalculation(int stick_num, float ema_value, float ema_previous) {
    return (adc_pairs[stick_num].ema_smoothing * ema_value) + ((1.0f - adc_pairs[stick_num].ema_smoothing) * ema_previous);
}

uint16_t AnalogInput::map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float AnalogInput::magnitudeCalculation(int stick_num, adc_instance & adc_inst) {
    adc_inst.x_magnitude = adc_inst.x_value - ANALOG_CENTER;
    adc_inst.y_magnitude = adc_inst.y_value - ANALOG_CENTER;
    return adc_pairs[stick_num].error_rate * std::sqrt((adc_inst.x_magnitude * adc_inst.x_magnitude) + (adc_inst.y_magnitude * adc_inst.y_magnitude));
}

void AnalogInput::setRadianDirection(adc_instance & adc_inst) {
    // should be range from 0 - 2pi
    float angle = std::atan2(adc_inst.y_magnitude, adc_inst.x_magnitude);
    if (angle < 0) {
        angle = angle + 2.0 * M_PI;
    }
    adc_inst.xy_radians = angle + adc_inst.angle_offset;
}

void AnalogInput::correctLinearity(adc_instance & adc_inst) {
    float diff = adc_inst.xy_radians - adc_inst.prev_xy_radians;
    while (diff < - M_PI){
        diff = diff + 2.0 * M_PI;
    }
    while (diff > M_PI) {
        diff = diff - 2.0 * M_PI;
    }
    if (std::abs(diff) > adc_inst.linearityMargin) {
        adc_inst.prev_xy_radians = adc_inst.xy_radians;
    } else {
        adc_inst.xy_radians = adc_inst.prev_xy_radians;
    }
}

void AnalogInput::snapToDirection(adc_instance & adc_inst) {
    for (int i=0; i< adc_inst.snap_direction_count; i++) {
        float diff = adc_inst.xy_radians - adc_inst.snap_directions[i].angle;
        while (diff < - M_PI){
            diff = diff + 2.0 * M_PI;
        }
        while (diff > M_PI) {
            diff = diff - 2.0 * M_PI;
        }
        if (std::abs(diff) < adc_inst.snap_directions[i].snap_margin) {
            adc_inst.xy_radians = adc_inst.snap_directions[i].angle;
            return;
        }
    }
}

void AnalogInput::radialDeadzone(int stick_num, adc_instance & adc_inst) {
    float scaling_factor = (adc_inst.xy_magnitude - adc_pairs[stick_num].in_deadzone) / (adc_pairs[stick_num].out_deadzone - adc_pairs[stick_num].in_deadzone);
    if (adc_pairs[stick_num].forced_circularity == true) {
        scaling_factor = std::fmin(scaling_factor, ANALOG_CENTER);
    }
    adc_inst.x_value = ((adc_inst.x_magnitude / adc_inst.xy_magnitude) * scaling_factor) + ANALOG_CENTER;
    adc_inst.y_value = ((adc_inst.y_magnitude / adc_inst.xy_magnitude) * scaling_factor) + ANALOG_CENTER;
    adc_inst.x_value = std::clamp(adc_inst.x_value, ANALOG_MINIMUM, ANALOG_MAX);
    adc_inst.y_value = std::clamp(adc_inst.y_value, ANALOG_MINIMUM, ANALOG_MAX);
}
