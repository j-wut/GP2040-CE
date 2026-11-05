// The parts of the generated config the key matrix uses (co-simulation only)
#pragma once
#include <stdint.h>
typedef uint16_t pb_size_t;
enum GpioAction { NONE = -10, RESERVED = -5, ASSIGNED_TO_ADDON = 0, BUTTON_PRESS_B1 = 5, _GpioAction_MAX = 138 };
struct GpioMappingInfo { GpioAction action; uint32_t customDpadMask; uint32_t customButtonMask; };
#define GpioMappingInfo_init_zero {}
struct KeyMatrixOptions {
    bool has_enabled; bool enabled;
    pb_size_t rowPins_count; int32_t rowPins[16];
    pb_size_t colPins_count; int32_t colPins[16];
    bool has_settleMicros; uint32_t settleMicros;
    bool has_rowDelayMicros; uint32_t rowDelayMicros;
};
