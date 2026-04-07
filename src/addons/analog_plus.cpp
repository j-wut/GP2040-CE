#include "addons/analog_plus.h"
#include "config.pb.h"
#include "enums.pb.h"
#include "hardware/adc.h"
#include "helper.h"
#include "storagemanager.h"
#include "drivermanager.h"

#include <math.h>

#define ADC_MAX ((1 << 12) - 1) // 4095
#define ADC_PIN_OFFSET 26

bool AnalogPlusInput::available() {
    return Storage::getInstance().getAddonOptions().analogPlusOptions.enabled;
}

void AnalogPlusInput::setup() {


    // Setup defaults and helpers
    for (int i = 0; i < ANALOG_PLUS_COUNT; i++) {
        joystick_state[i].xy_magnitude = 0f;
        joystick_state[i].x_reading = 0f;
        joystick_state[i].y_reading = 0f;
        joystick_state[i].xy_radians = 0f;
        joystick_state[i].prev_xy_magnitude = 0f;
        joystick_state[i].prev_x_reading = 0f;
        joystick_state[i].prev_y_reading = 0f;
        joystick_state[i].prev_xy_radians = 0f;
    }

    if (options.analog_configs[0].use_mux || options.analog_configs[1].use_mux) {
        switch(options.mux_channels) {
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

        for(int i = 0; i < selectPins; i++) {
            if ( options.select_pins[i] != -1 ) {
                gpio_init(options.select_pins[i]);
                gpio_set_dir(options.select_pins[i], GPIO_OUT);
                gpio_put(options.select_pins[i], 0);
            }
        }
    }

    // Intialize and auto center X/Y for each pair
    for (int i = 0; i < ANALOG_PLUS_COUNT; i++) {
        if(isValidPin(options.analog_configs[i].pin_x)) {
            adc_gpio_init(options.analog_configs[i].pin_x);
            if (options.analog_configs[i].auto_calibrate) {
                if(options.analog_configs[i].use_mux) {
                    selectChannel(options.analog_configs[i].channel_x);
                }
                adc_select_input(options.analog_configs[i].pin_x - ADC_PIN_OFFSET);
                joystick_state[i].cx = adc_read();
            } else {
                // if auto calibration is disabled, use middle as Center
                joystick_state[i].cx = ADC_MAX / 2;
            }
        }
        if(isValidPin(options.analog_configs[i].pin_y)) {
            adc_gpio_init(options.analog_configs[i].pin_y);
            if (options.analog_configs[i].auto_calibrate) {
                if(options.analog_configs[i].use_mux) {
                    selectChannel(options.analog_configs[i].channel_y);
                }
                adc_select_input(options.analog_configs[i].pin_y - ADC_PIN_OFFSET);
                options.analog_configs[i].cy = adc_read();
            } else {
                // if auto calibration is disabled, use middle as Center
                options.analog_configs[i].cy = ADC_MAX / 2;
            }
        }
    }
}

void AnalogPlusInput::process() {
    Gamepad * gamepad = Storage::getInstance().GetGamepad();
    
    uint32_t joystickMid = GAMEPAD_JOYSTICK_MID;
    uint32_t joystickMax = GAMEPAD_JOYSTICK_MAX;
    if ( DriverManager::getInstance().getDriver() != nullptr ) {
        joystickMid = DriverManager::getInstance().getDriver()->GetJoystickMidValue();
        joystickMax = joystickMid * 2; // 0x8000 mid must be 0x10000 max, but we reduce by 1 if we're maxed out
    }

    for(int i = 0; i < ANALOG_PLUS_COUNT; i++) {
        // Read X-Axis
        if (isValidPin(options.analog_configs[i].pin_x &&
            isValidPin(options.analog_configs[i].pin_y))) {

            // Read ADC values
            readXY(i);

            // Preprocessing Input Cleaning
            if (options.analog_configs[i].analog_invert == InvertMode::INVERT_X || 
                options.analog_configs[i].analog_invert == InvertMode::INVERT_XY) {
                joystick_state[i].x_reading = ADC_MAX - joystick_state[i].x_reading;
            }
            if (options.analog_configs[i].analog_invert == InvertMode::INVERT_Y || 
                options.analog_configs[i].analog_invert == InvertMode::INVERT_XY) {
                joystick_state[i].y_reading = ADC_MAX - joystick_state[i].y_reading;
            }

            if (options.analog_configs[i].analog_smoothing) {
                joystick_state[i].x_reading = emaCalculation(i, joystick_state[i].x_reading, joystick_state[i].x_ema);
                joystick_state[i].x_ema = joystick_state[i].x_reading;
                
                joystick_state[i].y_reading = emaCalculation(i, joystick_state[i].y_reading, joystick_state[i].y_ema);
                joystick_state[i].y_ema = joystick_state[i].y_reading;
            }

            // Apply Calibrations
            if (!options.analog_configs[i].auto_calibrate) {
                applyCalibration(i);
            } else {
                // If auto_calibrating map cx and cy to ADC_MAX/2
                if (joystick_state[i].x_reading > joystick_state[i].cx) {
                    joystick_state[i].x_reading = map(joystick_state[i].x_reading, joystick_state[i].cx, ADC_MAX, ADC_MAX / 2, ADC_MAX);
                } else if (joystick_state[i].x_reading == joystick_state[i].cx) {
                    joystick_state[i].x_reading = ADC_MAX / 2;
                } else {
                    joystick_state[i].x_reading = map(joystick_state[i].x_reading, 0, joystick_state[i].cx, 0, ADC_MAX / 2);
                }
                
                if (joystick_state[i].y_reading > joystick_state[i].cy) {
                    joystick_state[i].y_reading = map(joystick_state[i].y_reading, joystick_state[i].cy, ADC_MAX, ADC_MAX / 2, ADC_MAX);
                } else if (joystick_state[i].y_reading == joystick_state[i].cy) {
                    joystick_state[i].y_reading = ADC_MAX / 2;
                } else {
                    joystick_state[i].y_reading = map(joystick_state[i].y_reading, 0, joystick_state[i].cy, 0, ADC_MAX / 2);
                }
            }

            // Calculate XY Magnitude and angle
            calculatePolar(i);

            // Apply Angles
            if (!!options.analog_configs[i].linearity) {
                correctLinearity(i);
            }

            // Apply Angle Snapping
            if (options.analog_configs[i].analog_snapping) {
                snapToDirection(i);
            }

            // Apply Deadzones
            if (!!options.analog_configs[i].inner_deadzone) {
                if (options.analog_configs[i].forced_circularity) {
                    joystick_state[i].xy_magnitude = std::min(joystick_state[i].xy_magnitude, ADC_MAX/2)
                }
                float ratio = joystick_state[i].xy_magnitude / (ADC_MAX/2) * 100;
                if (ratio < options.analog_configs[i].inner_deadzone) {
                    joystick_state[i].xy_magnitude = 0f;
                }
            }

            float x_ratio = std::cos(joystick_state[i].xy_radians) * joystick_state[i].xy_magnitude / ADC_MAX / 2;
            float y_ratio = std::sin(joystick_state[i].xy_radians) * joystick_state[i].xy_magnitude / ADC_MAX / 2;

            // If MID is 0x8000, clamp our max to 0xFFFF incase we are at 0x10000. 0x7FFF will max at 0xFFFE
            uint16_t clampedX = (uint16_t)std::clamp((uint32_t)(joystickMid + joystickMid * x_ratio), (uint32_t)0x0, (uint32_t)0xFFFF);
            uint16_t clampedY = (uint16_t)std::clamp((uint32_t)(joystickMid + joystickMid * y_ratio), (uint32_t)0x0, (uint32_t)0xFFFF);

            if (options.analog_configs[i].analog_dpad == DpadMode::DPAD_MODE_LEFT_ANALOG) {
                gamepad->state.lx = clampedX;
                gamepad->state.ly = clampedY;
            } else if (options.analog_configs[i].analog_dpad == DpadMode::DPAD_MODE_RIGHT_ANALOG) {
                gamepad->state.rx = clampedX;
                gamepad->state.ry = clampedY;
            }
        }
    }
}

//Return diff of 2 radian angles. range from (-M_PI, M_PI)
float AnalogPlusInput::radianDiff(float angle1, float angle2) {
    float diff = joystick_state[i].xy_radians - joystick_state[i].prev_xy_radians;
    while (diff > M_PI) {
        diff -= 2 * M_PI;
    }
    while (diff < -M_PI) {
        diff += 2 * M_PI;
    }
    return diff;
}

void AnalogPlusInput::selectChannel(uint8_t channel) {
    for(int i = 0; i < selectPins; i++) {
        if ( selectPinArray[i] != -1 ) {
            gpio_put(selectPinArray[i], (channel >> i) & 0x01);
        }   
    }
}

void AnalogPlusInput::readXY(int joystick) {
    if (options.analog_config[joystick].use_mux) {
        selectChannel(options.analog_config[joystick].channel_x)
    }
    adc_select_input(options.analog_config[joystick].pin_x - ADC_PIN_OFFSET);
    joystick_state[joystick].x_reading = adc_read();

    if (options.analog_config[joystick].use_mux) {
        selectChannel(options.analog_config[joystick].channel_y)
    }
    adc_select_input(options.analog_config[joystick].pin_y - ADC_PIN_OFFSET);
    joystick_state[joystick].y_reading = adc_read();

}

void AnalogPlusInput::applyCalibration(int joystick) {
    AnalogCalibrationPoint[] calibration_points = options.analog_config[joystick].calibration_points;
    float weights[ANALOG_PLUS_CALIBRATION_COUNT_MAX];
    float totalWeight = 0f;
    int direction_count;
    for (direction_count=0; direction_count < ANALOG_PLUS_CALIBRATION_COUNT_MAX; direction_count++) {
        if (!options.analog_config[joystick].calibration_points[direction_count].enabled) {
            break;
        }

        float distance_sq = ((joystick_state[joystick].x_reading - calibration_points[direction_count].source_x)
                            * (joystick_state[joystick].x_reading - calibration_points[direction_count].source_x))
                            + ((joystick_state[joystick].y_reading - calibration_points[direction_count].source_y)
                            * (joystick_state[joystick].y_reading - calibration_points[direction_count].source_y));

        if (distance_sq == 0f) {
            weights[direction_count] = 1e6f;
        } else {
            weights[direction_count] = 1.0 / distance_sq;
        }
        totalWeight += weights[direction_count];
    }
    
    float result_X = 0f;
    float result_y = 0f;

    for (int i=0; i<direction_count; i++) {
        float normalized_weight = weights[i] / totalWeight;

        result_X += normalized_weight * (calibration_points[i].target_x + joystick_state[joystick].x_reading - calibration_points[i].source_x);
        result_y += normalized_weight * (calibration_points[i].target_y + joystick_state[joystick].y_reading - calibration_points[i].source_y)
    }

    joystick_state[joystick].x_reading = result_x;
    joystick_state[joystick].y_reading = result_y;
}

uint16_t AnalogPlusInput::map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float AnalogPlusInput::emaCalculation(int stick_num, float ema_value, float ema_previous) {
    return (options.analog_configs[stick_num].smoothing_factor * ema_value) + ((1.0f - options.analog_configs[stick_num].smoothing_factor) * ema_previous);
}

void AnalogPlusInput::calculatePolar(int joystick) {
    float x_magnitude = joystick_state[joystick].x_reading - ADC_MAX / 2;
    float y_magnitude = joystick_state[joystick].y_reading - ADC_MAX / 2;

    joystick_state[joystick].xy_magnitude =  std::sqrt((x_magnitude * x_magnitude) + (y_magnitude * y_magnitude));

    float angle = std::atan2(y_magnitude, x_magnitude) - adc_inst.angle_offset;
    if (angle < 0) {
        angle = angle + 2.0 * M_PI;
    }
    joystick_state[joystick] = angle;
}

void AnalogPlusInput::correctLinearity(int joystick) {
    float diff = radianDiff(joystick_state[i].xy_radians, joystick_state[i].prev_xy_radians)
    if (std::abs(abs) < std::abs(options.analog_configs[i].linearity)) {
        joystick_state[i].xy_radians = joystick_state[i].prev_xy_radians;
    } else {
        joystick_state[i].prev_xy_radians = joystick_state[i].xy_radians;
    }
}

void AnalogPlusInput::snapToDirection(int joystick) {
    for (int i=0; i < ANALOG_PLUS_SNAP_DIRECTION_COUNT_MAX; i++) {
        AnalogSnapDirection direction = options.analog_configs[joystick].snap_directions[i];
        if(!direction.enabled) {
            continue;
        }
        float diff = radianDiff(joystick_state[joystick].xy_radians, direction.angle)
        if (std::abs(diff) < std::abs(direction.snap_margin)) {
            adc_inst.xy_radians = direction.angle;
            return;
        }
    }
}

void AnalogPlusInput::radialDeadzone(int stick_num, adc_instance & adc_inst) {
    float scaling_factor = (adc_inst.xy_magnitude - options.analog_configs[stick_num].in_deadzone) / (options.analog_configs[stick_num].out_deadzone - options.analog_configs[stick_num].in_deadzone);
    if (options.analog_configs[stick_num].forced_circularity == true) {
        scaling_factor = std::fmin(scaling_factor, ANALOG_CENTER);
    }
    adc_inst.x_value = ((adc_inst.x_magnitude / adc_inst.xy_magnitude) * scaling_factor) + ANALOG_CENTER;
    adc_inst.y_value = ((adc_inst.y_magnitude / adc_inst.xy_magnitude) * scaling_factor) + ANALOG_CENTER;
    adc_inst.x_value = std::clamp(adc_inst.x_value, ANALOG_MINIMUM, ANALOG_MAX);
    adc_inst.y_value = std::clamp(adc_inst.y_value, ANALOG_MINIMUM, ANALOG_MAX);
}
