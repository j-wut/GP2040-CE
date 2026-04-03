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
	DISABLED = 0,
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

export interface AnalogCalibrationPoint {
	source: JoystickPosition,
	target: JoystickPosition
}

export interface AnalogOptions {
    [keyName: string]: any;
	pin_x: number;
	pin_y: number;
	invert_mode: AnalogInvertMode;
	analog_mode: AnalogMode;
	forced_circularity: boolean;
	inner_deadzone: number;
	auto_calibrate: boolean;
	calibration_points: AnalogCalibrationPoint[];
	analog_smoothing: boolean;
	smoothing_factor: number;
	analog_error: number;

	use_mux: boolean;
	channel_x: number;
	channel_y: number;
	linearity: number;
	angle_snapping: boolean;
	snap_directions: AnalogSnapDirection[];
}
export interface AnalogPluginOptions {
    [keyName: string]: any;
	AnalogInputEnabled: boolean;
	
	analog_mux_channels: number;
	analogSelectPin0: number;
	analogSelectPin1: number;
	analogSelectPin2: number;
	analogSelectPin3: number;

	analogOptions: AnalogOptions[];
}