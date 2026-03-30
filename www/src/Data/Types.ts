export const ADC_MAX = 4095;

export interface GetJoystickPositionRequest {
    channels: number;
    selectPins: number[];
    xChannel: number;
    xAdcPin: number;
    yChannel: number;
    yAdcPin: number;
}
export interface JoystickPosition {
    x: number;
    y: number;
}

export enum AnalogMode {
    LEFT_ANALOG = 1,
    RIGHT_ANALOG = 2,
}

export enum AnalogInvertMode {
    NONE,
    X_AXIS,
    Y_AXIS,
    XY_AXIS,
}

export interface AnalogSnapDirection {
    angle: number;
    snap_margin: number;
    activation: number;
    release: number;
}

export interface AnalogOptions {
    [keyName: string]: any;
	AnalogInputEnabled: boolean;
	analogAdc1PinX: number;
	analogAdc1PinY: number;
	analogAdc1Mode: AnalogMode;
	analogAdc1Invert: AnalogInvertMode;
	analogAdc2PinX: number;
	analogAdc2PinY: number;
	analogAdc2Mode: AnalogMode;
	analogAdc2Invert: AnalogInvertMode;
	forced_circularity: boolean;
	forced_circularity2: boolean;
	inner_deadzone: number;
	inner_deadzone2: number;
	outer_deadzone: number;
	outer_deadzone2: number;
	auto_calibrate: boolean;
	auto_calibrate2: boolean;
	joystickCenterX: number;
	joystickCenterY: number;
	joystickCenterX2: number;
	joystickCenterY2: number;
	analog_smoothing: boolean;
	analog_smoothing2: boolean;
	smoothing_factor: number;
	smoothing_factor2: number;
	analog_error: number;
	analog_error2: number;

	analog_mux_channels: number;
	analogSelectPin0: number;
	analogSelectPin1: number;
	analogSelectPin2: number;
	analogSelectPin3: number;

	analog_mux_1: boolean;
	analog_channel_x_1: number;
	analog_channel_y_1: number;
	analog_mux_2: boolean;
	analog_channel_x_2: number;
	analog_channel_y_2: number;

    analog_linearity_1: boolean;
    analog_linearity_margin_1: number;
    analog_linearity_2: boolean;
    analog_linearity_margin_2: number;

    analog_angle_snapping_1: boolean;
    analog_directions_1: AnalogSnapDirection[];
    
    analog_angle_snapping_2: boolean;
    analog_directions_2: AnalogSnapDirection[];

    analog_rotation_offset_1: number;
    analog_rotation_offset_2: number;
}