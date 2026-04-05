#ifndef _Analog_Plus_H
#define _Analog_Plus_H

#include "gpaddon.h"
#include "GamepadEnums.h"
#include "BoardConfig.h"
#include "enums.pb.h"
#include "types.h"

#ifndef ANALOG_PLUS_ENABLED
#define ANALOG_PLUS_ENABLED 0
#endif

#ifndef ANALOG_PLUS_COUNT
#define ANALOG_PLUS_COUNT 2
#endif

#ifndef ANALOG_PLUS_CHANNELS_DEFAULT
#define ANALOG_PLUS_CHANNELS_DEFAULT 1
#endif

#ifndef ANALOG_PLUS_SELECT_PIN_MAX
#define ANALOG_PLUS_SELECT_PIN_MAX 4
#endif

#ifndef ANALOG_PLUS_SELECT_PIN_DEFAULT
#define ANALOG_PLUS_SELECT_PIN_DEFAULT -1
#endif

#ifndef ANALOG_PLUS_SELECT_PIN_0
#define ANALOG_PLUS_SELECT_PIN_0 ANALOG_PLUS_SELECT_PIN_DEFAULT
#endif

#ifndef ANALOG_PLUS_SELECT_PIN_1
#define ANALOG_PLUS_SELECT_PIN_1 ANALOG_PLUS_SELECT_PIN_DEFAULT
#endif

#ifndef ANALOG_PLUS_SELECT_PIN_2
#define ANALOG_PLUS_SELECT_PIN_2 ANALOG_PLUS_SELECT_PIN_DEFAULT
#endif

#ifndef ANALOG_PLUS_SELECT_PIN_3
#define ANALOG_PLUS_SELECT_PIN_3 ANALOG_PLUS_SELECT_PIN_DEFAULT
#endif


#ifndef ANALOG_PLUS_MODE_DEFAULT
#define ANALOG_PLUS_MODE_DEFAULT AnalogMode.DISABLED
#endif

#ifndef ANALOG_PLUS_MODE_1
#define ANALOG_PLUS_MODE_1 ANALOG_PLUS_MODE_DEFAULT
#endif

#ifndef ANALOG_PLUS_USE_MUX_DEFAULT
#define ANALOG_PLUS_USE_MUX_DEFAULT 0
#endif

#ifndef ANALOG_PLUS_USE_MUX_1
#define ANALOG_PLUS_USE_MUX_1 ANALOG_PLUS_USE_MUX_DEFAULT
#endif

#ifndef ANALOG_PLUS_CHANNEL_X_DEFAULT
#define ANALOG_PLUS_CHANNEL_X_DEFAULT -1
#endif

#ifndef ANALOG_PLUS_CHANNEL_X_1
#define ANALOG_PLUS_CHANNEL_X_1 ANALOG_PLUS_CHANNEL_X_DEFAULT
#endif

#ifndef ANALOG_PLUS_CHANNEL_Y_DEFAULT
#define ANALOG_PLUS_CHANNEL_Y_DEFAULT -1
#endif

#ifndef ANALOG_PLUS_CHANNEL_Y_1
#define ANALOG_PLUS_CHANNEL_Y_1 ANALOG_PLUS_CHANNEL_Y_DEFAULT
#endif

#ifndef ANALOG_PLUS_PIN_X_DEFAULT
#define ANALOG_PLUS_PIN_X_DEFAULT -1
#endif

#ifndef ANALOG_PLUS_PIN_X_1
#define ANALOG_PLUS_PIN_X_1 ANALOG_PLUS_PIN_X_DEFAULT
#endif

#ifndef ANALOG_PLUS_PIN_Y_DEFAULT
#define ANALOG_PLUS_PIN_Y_DEFAULT -1
#endif

#ifndef ANALOG_PLUS_PIN_Y_1
#define ANALOG_PLUS_PIN_Y_1 ANALOG_PLUS_PIN_Y_DEFAULT
#endif

#ifndef ANALOG_PLUS_INVERT_MODE_DEFAULT
#define ANALOG_PLUS_INVERT_MODE_DEFAULT AnalogInvertMode.NONE

#ifndef ANALOG_PLUS_INVERT_MODE_1
#define ANALOG_PLUS_INVERT_MODE_1 ANALOG_PLUS_INVERT_MODE_DEFAULT
#endif

#ifndef ANALOG_PLUS_FORCED_CIRCULARITY_DEFAULT
#define ANALOG_PLUS_FORCED_CIRCULARITY_DEFAULT 0
#endif

#ifndef ANALOG_PLUS_FORCED_CIRCULARITY_1
#define ANALOG_PLUS_FORCED_CIRCULARITY_1 ANALOG_PLUS_FORCED_CIRCULARITY_DEFAULT
#endif

#ifndef ANALOG_PLUS_INNER_DEADZONE_DEFAULT
#define ANALOG_PLUS_INNER_DEADZONE_DEFAULT 5
#endif

#ifndef ANALOG_PLUS_INNER_DEADZONE_1
#define ANALOG_PLUS_INNER_DEADZONE_1 ANALOG_PLUS_INNER_DEADZONE_DEFAULT
#endif

#ifndef ANALOG_PLUS_OUTER_DEADZONE_DEFAULT
#define ANALOG_PLUS_OUTER_DEADZONE_DEFAULT 95
#endif

#ifndef ANALOG_PLUS_OUTER_DEADZONE_1
#define ANALOG_PLUS_OUTER_DEADZONE_1 ANALOG_PLUS_OUTER_DEADZONE_DEFAULT
#endif

#ifndef ANALOG_PLUS_AUTO_CALIBRATE_DEFAULT
#define ANALOG_PLUS_AUTO_CALIBRATE_DEFAULT 0
#endif

#ifndef ANALOG_PLUS_AUTO_CALIBRATE_1
#define ANALOG_PLUS_AUTO_CALIBRATE_1 ANALOG_PLUS_AUTO_CALIBRATE_DEFAULT
#endif

#ifndef ANALOG_PLUS_CALIBRATION_COUNT_MAX
#define ANALOG_PLUS_CALIBRATION_COUNT_MAX 13
#endif

#ifndef ANALOG_PLUS_ANALOG_SMOOTHING_DEFAULT
#define ANALOG_PLUS_ANALOG_SMOOTHING_DEFAULT 0
#endif

#ifndef ANALOG_PLUS_ANALOG_SMOOTHING_1
#define ANALOG_PLUS_ANALOG_SMOOTHING_1 ANALOG_PLUS_ANALOG_SMOOTHING_DEFAULT
#endif

#ifndef ANALOG_PLUS_SMOOTHING_FACTOR_DEFAULT
#define ANALOG_PLUS_SMOOTHING_FACTOR_DEFAULT 5
#endif

#ifndef ANALOG_PLUS_SMOOTHING_FACTOR_1
#define ANALOG_PLUS_SMOOTHING_FACTOR_1 ANALOG_PLUS_SMOOTHING_FACTOR_DEFAULT
#endif

#ifndef ANALOG_PLUS_ANALOG_ERROR_DEFAULT
#define ANALOG_PLUS_ANALOG_ERROR_DEFAULT 1000
#endif

#ifndef ANALOG_PLUS_ANALOG_ERROR_1
#define ANALOG_PLUS_ANALOG_ERROR_1 ANALOG_PLUS_ANALOG_ERROR_DEFAULT
#endif

#ifndef ANALOG_PLUS_LINEARITY_DEFAULT
#define ANALOG_PLUS_LINEARITY_DEFAULT 0
#endif

#ifndef ANALOG_PLUS_LINEARITY_1
#define ANALOG_PLUS_LINEARITY_1 ANALOG_PLUS_LINEARITY_DEFAULT
#endif

#ifndef ANALOG_PLUS_SNAP_DIRECTION_COUNT_MAX
#define ANALOG_PLUS_SNAP_DIRECTION_COUNT_MAX 16
#endif

// Analog Module Name
#define AnalogPlusName "AnalogPLUS"

typedef struct
{
    uint16_t cx;
    uint16_t cy;
    
    float x_reading;
    float y_reading;

    float x_ema;
    float y_ema;

    float prev_x_reading;
    float prev_y_reading;

    float xy_magnitude;
    float xy_radians;

    float prev_xy_magnitude;
    float prev_xy_radians;

} AnalogState;

class AnalogPlusInput : public GPAddon {
public:
    virtual bool available();
    virtual void setup();       // Analog Setup
    virtual void process();     // Analog Process
    virtual void preprocess() {}
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return AnalogPlusName; }
private:
    void selectChannel(uint8_t channel);
    float readPin(int stick_num, Pin_t pin, uint16_t center);
    float emaCalculation(int stick_num, float ema_value, float ema_previous);
    uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
    float magnitudeCalculation(int stick_num, adc_instance & adc_inst);
    void setRadianDirection(adc_instance & adc_inst);
    void correctLinearity(adc_instance & adc_inst);
    void snapToDirection(adc_instance & adc_inst);
    void radialDeadzone(int stick_num, adc_instance & adc_inst);
    AnalogState joystick_state[ANALOG_PLUS_COUNT];
    int selectPins;
    const AnalogPlusOptions& options = Storage::getInstance().getAddonOptions().analogPlusOptions;
};

#endif  // _Analog_H_
