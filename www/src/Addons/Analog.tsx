import * as d3 from "d3";
import { useContext, useEffect, useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { Button, Col, FormCheck, Row, Tab, Tabs } from 'react-bootstrap';
import * as yup from 'yup';

import Section from '../Components/Section';
import FormSelect from '../Components/FormSelect';
import { ANALOG_PINS } from '../Data/Buttons';
import AnalogPinOptions from '../Components/AnalogPinOptions';
import { AppContext } from '../Contexts/AppContext';
import FormControl from '../Components/FormControl';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';
import { ADC_MAX, AnalogInvertMode, AnalogMode, AnalogOptions, AnalogSnapDirection, GetJoystickPositionRequest, JoystickPosition } from '../Data/Types';
import WebApi from '../Services/WebApi';
import { set } from "lodash";

const ANALOG_STICK_MODES = [
	{ label: 'Left Analog', value: AnalogMode.LEFT_ANALOG },
	{ label: 'Right Analog', value: AnalogMode.RIGHT_ANALOG },
];

const INVERT_MODES = [
	{ label: 'None', value: AnalogInvertMode.NONE},
	{ label: 'X Axis', value: AnalogInvertMode.X_AXIS },
	{ label: 'Y Axis', value: AnalogInvertMode.Y_AXIS },
	{ label: 'X/Y Axis', value: AnalogInvertMode.XY_AXIS },
];

const ANALOG_ERROR_RATES = [
	{ label: '0%', value: 1000 },
	{ label: '1%', value: 990 },
	{ label: '2%', value: 979 },
	{ label: '3%', value: 969 },
	{ label: '4%', value: 958 },
	{ label: '5%', value: 946 },
	{ label: '6%', value: 934 },
	{ label: '7%', value: 922 },
	{ label: '8%', value: 911 },
	{ label: '9%', value: 900 },
	{ label: '10%', value: 890 },
	{ label: '11%', value: 876 },
	{ label: '12%', value: 863 },
	{ label: '13%', value: 848 },
	{ label: '14%', value: 834 },
	{ label: '15%', value: 821 },
];

export const analogScheme = {
	AnalogInputEnabled: yup.number().required().label('Analog Input Enabled'),
	analogAdc1PinX: yup
		.number()
		.label('Analog Stick 1 Pin X')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogAdc1PinY: yup
		.number()
		.label('Analog Stick 1 Pin Y')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogAdc1Mode: yup
		.number()
		.label('Analog Stick 1 Mode')
		.validateSelectionWhenValue('AnalogInputEnabled', ANALOG_STICK_MODES),
	analogAdc1Invert: yup
		.number()
		.label('Analog Stick 1 Invert')
		.validateSelectionWhenValue('AnalogInputEnabled', INVERT_MODES),
	analogAdc2PinX: yup
		.number()
		.label('Analog Stick 2 Pin X')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogAdc2PinY: yup
		.number()
		.label('Analog Stick 2 Pin Y')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogAdc2Mode: yup
		.number()
		.label('Analog Stick 2 Mode')
		.validateSelectionWhenValue('AnalogInputEnabled', ANALOG_STICK_MODES),
	analogAdc2Invert: yup
		.number()
		.label('Analog Stick 2 Invert')
		.validateSelectionWhenValue('AnalogInputEnabled', INVERT_MODES),
	forced_circularity: yup
		.number()
		.label('Force Circularity')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	forced_circularity2: yup
		.number()
		.label('Force Circularity')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	inner_deadzone: yup
		.number()
		.label('Inner Deadzone Size (%)')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 100),
	inner_deadzone2: yup
		.number()
		.label('Inner Deadzone Size (%)')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 100),
	outer_deadzone: yup
		.number()
		.label('Outer Deadzone Size (%)')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 100),
	outer_deadzone2: yup
		.number()
		.label('Outer Deadzone Size (%)')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 100),
	auto_calibrate: yup
		.number()
		.label('Auto Calibration')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	auto_calibrate2: yup
		.number()
		.label('Auto Calibration')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	analog_smoothing: yup
		.number()
		.label('Analog Smoothing')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	analog_smoothing2: yup
		.number()
		.label('Analog Smoothing 2')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	smoothing_factor: yup
		.number()
		.label('Smoothing Factor')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 100),
	smoothing_factor2: yup
		.number()
		.label('Smoothing Factor 2')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 100),
	analog_error: yup
		.number()
		.label('Error Rate')
		.validateSelectionWhenValue('AnalogInputEnabled', ANALOG_ERROR_RATES),
	analog_error2: yup
		.number()
		.label('Error Rate 2')
		.validateSelectionWhenValue('AnalogInputEnabled', ANALOG_ERROR_RATES),
	joystickCenterX: yup
		.number()
		.label('Joystick Center X')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 4095),
	joystickCenterY: yup
		.number()
		.label('Joystick Center Y')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 4095),
	joystickCenterX2: yup
		.number()
		.label('Joystick Center X2')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 4095),
	joystickCenterY2: yup
		.number()
		.label('Joystick Center Y2')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 4095),

	analog_mux_channels: yup
		.number()
		.label('Analog Mux Channels')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 16),
	analogSelectPin0: yup
		.number()
		.label('Analog Select Pin 0')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogSelectPin1: yup
		.number()
		.label('Analog Select Pin 1')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogSelectPin2: yup
		.number()
		.label('Analog Select Pin 2')
		.validatePinWhenValue('AnalogInputEnabled'),
	analogSelectPin3: yup
		.number()
		.label('Analog Select Pin 3')
		.validatePinWhenValue('AnalogInputEnabled'),
	analog_mux_1: yup
		.number()
		.label('Analog 1 Mux Enabled')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	analog_channel_x_1: yup
		.number()
		.label('Analog 1 X Channel')
		.validateRangeWhenValue('analog_mux_1', 0, 16),
	analog_channel_y_1: yup
		.number()
		.label('Analog 1 Y Channel')
		.validateRangeWhenValue('analog_mux_1', 0, 16),
	analog_mux_2: yup
		.number()
		.label('Analog 2 Mux Enabled')
		.validateRangeWhenValue('AnalogInputEnabled', 0, 1),
	analog_channel_x_2: yup
		.number()
		.label('Analog 2 X Channel')
		.validateRangeWhenValue('analog_mux_2', 0, 16),
	analog_channel_y_2: yup
		.number()
		.label('Analog 2 Y Channel')
		.validateRangeWhenValue('analog_mux_2', 0, 16),
};

export const analogState: AnalogOptions = {
	AnalogInputEnabled: false,
	analogAdc1PinX: -1,
	analogAdc1PinY: -1,
	analogAdc1Mode: 1,
	analogAdc1Invert: 0,
	analogAdc2PinX: -1,
	analogAdc2PinY: -1,
	analogAdc2Mode: 2,
	analogAdc2Invert: 0,
	forced_circularity: false,
	forced_circularity2: false,
	inner_deadzone: 5,
	inner_deadzone2: 5,
	outer_deadzone: 95,
	outer_deadzone2: 95,
	auto_calibrate: false,
	auto_calibrate2: false,
	joystickCenterX: 0,
	joystickCenterY: 0,
	joystickCenterX2: 0,
	joystickCenterY2: 0,
	analog_smoothing: false,
	analog_smoothing2: false,
	smoothing_factor: 5,
	smoothing_factor2: 5,
	analog_error: 1,
	analog_error2: 1,

	analog_mux_channels: 8,
	analogSelectPin0: -1,
	analogSelectPin1: -1,
	analogSelectPin2: -1,
	analogSelectPin3: -1,

	analog_mux_1: false,
	analog_channel_x_1: -1,
	analog_channel_y_1: -1,

	analog_mux_2: false,
	analog_channel_x_2: -1,
	analog_channel_y_2: -1,
	analog_linearity_1: false,
	analog_linearity_margin_1: 0,
	analog_linearity_2: false,
	analog_linearity_margin_2: 0,
	analog_angle_snapping_1: false,
	analog_directions_1: [],
	analog_angle_snapping_2: false,
	analog_directions_2: [],
	analog_rotational_offset_1: 0,
	analog_rotational_offset_2: 0
};

const colors = [
	"red",
	"orange",
	"yellow",
	"green",
	"blue",
	"indigo",
	"violet",
	"teal",
	"cyan",
	"magenta",
	"azure",
	"deeppink",
	"navy",
	"lawngreen",
	"coral",
	"aliceblue"
]

const Analog = ({ errors, handleChange, handleCheckbox, setFieldValue }: AddonPropTypes) => {
	const [analogConfig , setAnalogConfig] = useState(analogState);
	const timerId = useRef<number>();
	const [center, setCenter] = useState({x:0,y:0});
	const [joystickPosition, setJoystickPosition] = useState({x:0,y:0});
	const [correctedPosition, setCorrectedPosition] = useState({x:0,y:0});
	const { usedPins } = useContext(AppContext);
	const { t } = useTranslation();

	const toggleConfig = (e)=> {
		console.log(e);
		console.log(e.target.value);
		let changedAttributeName: string = e.target.name;
		const newConfigState: AnalogOptions = {...analogConfig};
		newConfigState[changedAttributeName] = !(e.target.value == "on");
		setAnalogConfig(newConfigState);
	}

	const setConfig = (field: string, value: any)=>{
		const newConfigState: AnalogOptions = {...analogConfig};
		newConfigState[field] = value;
		setAnalogConfig(newConfigState);
	}

	useEffect(()=>{
		const getAnalogConfig = async ()=> await WebApi.getAnalogSettings();
		getAnalogConfig().then(
			setAnalogConfig
		);
	}, [])

	useEffect(()=>{
		if(analogConfig.auto_calibrate){
			WebApi.getJoystickPosition({
				channels: analogConfig.analog_mux_channels,
				selectPins: [analogConfig.analogSelectPin0, analogConfig.analogSelectPin1, analogConfig.analogSelectPin2, analogConfig.analogSelectPin3],
				xChannel: analogConfig.analog_channel_x_1,
				xAdcPin: analogConfig.analogAdc1PinX,
				yChannel: analogConfig.analog_channel_y_1,
				yAdcPin: analogConfig.analogAdc1PinY
			}).then(setCenter);
		} else {
			setCenter({x: analogConfig.joystickCenterX, y: analogConfig.joystickCenterY});
		}
	}, [analogConfig])

	useEffect(()=>{
		let magnitude_x = joystickPosition.x - center.x;
		let magnitude_y = joystickPosition.y - center.y;
		let magnitude_xy = Math.sqrt(magnitude_x * magnitude_x + magnitude_y * magnitude_y);
		let radians =  Math.atan2(magnitude_y, magnitude_x) - analogConfig.analog_rotation_offset_1;
		setCorrectedPosition({x: magnitude_xy * Math.cos(radians)+ center.x, y: magnitude_xy * Math.sin(radians)+center.y});
	}, [joystickPosition])

	const readJoystickPosition = async () => {
		let position = await WebApi.getJoystickPosition({
			channels: analogConfig.analog_mux_channels,
			selectPins: [analogConfig.analogSelectPin0, analogConfig.analogSelectPin1, analogConfig.analogSelectPin2, analogConfig.analogSelectPin3],
			xChannel: analogConfig.analog_channel_x_1,
			xAdcPin: analogConfig.analogAdc1PinX,
			yChannel: analogConfig.analog_channel_y_1,
			yAdcPin: analogConfig.analogAdc1PinY
		});
		if (analogConfig.analogAdc1Invert == AnalogInvertMode.XY_AXIS || analogConfig.analogAdc1Invert == AnalogInvertMode.X_AXIS) {
			position.x = ADC_MAX - position.x;
		}
		if (analogConfig.analogAdc1Invert == AnalogInvertMode.XY_AXIS || analogConfig.analogAdc1Invert == AnalogInvertMode.Y_AXIS) {
			position.y = ADC_MAX - position.y;
		}
		setJoystickPosition(position);
	}

	const stopVisualization = async () => {
		if (timerId)
			clearInterval(timerId.current);
	};

	const startVisualization = async () => {
		if (analogConfig.auto_calibrate) {
			WebApi.getJoystickPosition({
				channels: analogConfig.analog_mux_channels,
				selectPins: [analogConfig.analogSelectPin0, analogConfig.analogSelectPin1, analogConfig.analogSelectPin2, analogConfig.analogSelectPin3],
				xChannel: analogConfig.analog_channel_x_1,
				xAdcPin: analogConfig.analogAdc1PinX,
				yChannel: analogConfig.analog_channel_y_1,
				yAdcPin: analogConfig.analogAdc1PinY
			}).then(setCenter)
		}
		if (timerId.current)
			clearInterval(timerId.current);
		const intervalId = setInterval(() => {
			readJoystickPosition();
		}, 50);
		timerId.current = intervalId;
	}

	const AnalogVisualization = ()=> {
		let directions = analogConfig.analog_directions_1;
		let pathD = directions.reverse().map((d) =>d3.arc().innerRadius(d.activation).outerRadius(d.release).startAngle(d.angle-d.snap_margin).endAngle(d.angle+d.snap_margin)());

		return (
			<svg viewBox={`-100 -100 200 200`}> {/* at some point have to figure out why this is dumb as fuck */}
				<circle fill="white" stroke="black" strokeWidth="1" cx="0" cy="0" r={100}/>
				{
				pathD.map((d, i) => <path x="0" y="0"  fillOpacity="40%" strokeWidth="2" d={d}/>)
				}
				<circle fill="black" fillOpacity="50%" cx={(joystickPosition.x - center.x)/(ADC_MAX - center.x)*100} cy={(joystickPosition.y - center.y)/(ADC_MAX - center.y)*100} r={3}/>
				<circle fill="crimson" fillOpacity="100%" cx={(correctedPosition.x - center.x)/(ADC_MAX - center.x)*100} cy={(correctedPosition.y - center.y)/(ADC_MAX - center.y)*100} r={3}/>
			</svg>
		);
	}

	
	const getCalibrationValues = async (params: GetJoystickPositionRequest): Promise <{
		step: number;
		direction: string;
		x: number;
		y: number;
	}[] | void> => {
		const steps = [
			{ direction: t('AddonsConfig:analog-calibration-direction-top-left'), position: 'top-left' },
			{ direction: t('AddonsConfig:analog-calibration-direction-top-right'), position: 'top-right' },
			{ direction: t('AddonsConfig:analog-calibration-direction-bottom-left'), position: 'bottom-left' },
			{ direction: t('AddonsConfig:analog-calibration-direction-bottom-right'), position: 'bottom-right' }
		];
		let calibrationValues = []
		
		for (let i = 0; i < steps.length; i++) {
			const step = steps[i];
			const stepNumber = i + 1;
			
			
			// Show confirmation dialog
			const userConfirmed = confirm(
				t('AddonsConfig:analog-calibration-step-title', { step: stepNumber }) + '\n\n' +
				t('AddonsConfig:analog-calibration-step-instruction', { stick: '1', direction: step.direction }) + '\n\n' +
				t('AddonsConfig:analog-calibration-step-confirm', { stick: '1', step: stepNumber })
			);
			
			if (!userConfirmed) {
				alert(t('AddonsConfig:analog-calibration-cancelled'));
				return;
			}
			
			
			// Read current center value
			console.log(`Fetching joystick 1 center for step ${stepNumber}...`);
			const data = await WebApi.getJoystickPosition(params);

			console.log('Response data:', data);
			
			calibrationValues.push({
				step: stepNumber,
				direction: step.direction,
				x: data.x || 0,
				y: data.y || 0
			});
			
			console.log(`Step ${stepNumber} completed:`, calibrationValues[i]);
		}

		return calibrationValues;
	}
	
	const CHANNELS_OPTIONS = {
		1: t('HETrigger:direct-no-mux'),
		4: t('HETrigger:4-channels'),
		8: t('HETrigger:8-channels'),
		16: t('HETrigger:16-channels'),
	};

	const availableAnalogPins = ANALOG_PINS.filter(
		(pin) => !usedPins?.includes(pin),
	);

	return (
		<Section title={
			<a
				href="https://gp2040-ce.info/add-ons/analog"
				target="_blank"
				className="text-reset text-decoration-none"
			>
				{t('AddonsConfig:analog-header-text')}
			</a>
		}
		>
			<div id="AnalogInputOptions" hidden={!analogConfig.AnalogInputEnabled}>
				<div className="alert alert-info" role="alert">
					{t('AddonsConfig:analog-warning')}
				</div>
				<div className="alert alert-success" role="alert">
					{t('AddonsConfig:analog-available-pins-text', {
						pins: availableAnalogPins.join(', '),
					})}
				</div>
				
				{(analogConfig.analog_mux_1 || analogConfig.analog_mux_2) && <>
					<Row className="mt-2">
						<FormSelect
							label={t('HETrigger:multiplexer-channel-select')}
							name="analog_mux_channels"
							className="form-select-sm"
							groupClassName="col-sm-3 mb-3"
							value={analogConfig.analog_mux_channels}
							error={errors.analog_mux_channels}
							isInvalid={Boolean(errors.analog_mux_channels)}
							onChange={handleChange}
						>
							{Object.entries(CHANNELS_OPTIONS).map(([num, label], i) => (
								<option key={`channels-per-mux-option-${i}`} value={num}>
									{label}
								</option>
							))}
						</FormSelect>
					</Row>
					<Row className="mb-3">
						<FormControl
							type="number"
							label={t('HETrigger:select-pin-0')}
							name="analogSelectPin0"
							hidden={analogConfig.analog_mux_channels < 4}
							className="form-select-sm"
							groupClassName="col-sm-2 mb-3"
							value={analogConfig.analogSelectPin0}
							error={errors.analogSelectPin0}
							isInvalid={Boolean(errors.analogSelectPin0)}
							onChange={handleChange}
							min={-1}
							max={29}
						/>
						<FormControl
							type="number"
							label={t('HETrigger:select-pin-1')}
							name="analogSelectPin1"
							hidden={analogConfig.analog_mux_channels < 4}
							className="form-select-sm"
							groupClassName="col-sm-2 mb-3"
							value={analogConfig.analogSelectPin1}
							error={errors.analogSelectPin1}
							isInvalid={Boolean(errors.analogSelectPin1)}
							onChange={handleChange}
							min={-1}
							max={29}
						/>
						<FormControl
							type="number"
							label={t('HETrigger:select-pin-2')}
							name="analogSelectPin2"
							hidden={analogConfig.analog_mux_channels < 8}
							className="form-select-sm"
							groupClassName="col-sm-2 mb-3"
							value={analogConfig.analogSelectPin2}
							error={errors.analogSelectPin2}
							isInvalid={Boolean(errors.analogSelectPin2)}
							onChange={handleChange}
							min={-1}
							max={29}
						/>
						<FormControl
							type="number"
							label={t('HETrigger:select-pin-3')}
							name="analogSelectPin3"
							hidden={analogConfig.analog_mux_channels < 16}
							className="form-select-sm"
							groupClassName="col-sm-2 mb-3"
							value={analogConfig.analogSelectPin3}
							error={errors.analogSelectPin3}
							isInvalid={Boolean(errors.analogSelectPin3)}
							onChange={handleChange}
							min={-1}
							max={29}
						/>
					</Row>
				</>}

				<Tabs
					defaultActiveKey="analog1Config"
					id="analogConfigTabs"
					className="mb-3 pb-0"
					fill
				>
					<Tab
						key="analog1Config"
						eventKey="analog1Config"
						title={t('AddonsConfig:analog-adc-1')}
					>
						<Row>
						<Col>
						<Row className="mb-3">
							<FormCheck
								label={"use mux"}
								type="switch"
								id="use_mux"
								className="col-sm-3 ms-3"
								isInvalid={false}
								checked={Boolean(analogConfig.analog_mux_1)}
								onChange={(e) => {
									console.log(e);
									handleCheckbox('analog_mux_1');
									handleChange(e);
								}}
							/>
						</Row>
						{analogConfig.analog_mux_1 && <Row className="mb-3">
							<FormControl
								type="number"
								label={"x channel"}
								name="analog_channel_x_1"
								className="form-select-sm"
								groupClassName="col-sm-2 mb-3"
								value={analogConfig.analog_channel_x_1}
								error={errors.analog_channel_x_1}
								isInvalid={Boolean(errors.analog_channel_x_1)}
								onChange={handleChange}
								min={0}
								max={analogConfig.analog_mux_channels - 1}
							/>
							<FormControl
								type="number"
								label={"y channel"}
								name="analog_channel_y_1"
								className="form-select-sm"
								groupClassName="col-sm-2 mb-3"
								value={analogConfig.analog_channel_y_1}
								error={errors.analog_channel_y_1}
								isInvalid={Boolean(errors.analog_channel_y_1)}
								onChange={handleChange}
								min={0}
								max={analogConfig.analog_mux_channels - 1}
							/>
						</Row>}
						<Row className="mb-3">
							<FormSelect
								label={t('AddonsConfig:analog-adc-1-pin-x-label')}
								name="analogAdc1PinX"
								className="form-select-sm"
								groupClassName="col-sm-3 mb-3"
								value={analogConfig.analogAdc1PinX}
								error={errors.analogAdc1PinX}
								isInvalid={Boolean(errors.analogAdc1PinX)}
								onChange={handleChange}
							>
								<AnalogPinOptions />
							</FormSelect>
							<FormSelect
								label={t('AddonsConfig:analog-adc-1-pin-y-label')}
								name="analogAdc1PinY"
								className="form-select-sm"
								groupClassName="col-sm-3 mb-3"
								value={analogConfig.analogAdc1PinY}
								error={errors.analogAdc1PinY}
								isInvalid={Boolean(errors.analogAdc1PinY)}
								onChange={handleChange}
							>
								<AnalogPinOptions />
							</FormSelect>
							<Row className="mb-3">
								<FormSelect
									label={t('AddonsConfig:analog-adc-1-mode-label')}
									name="analogAdc1Mode"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.analogAdc1Mode}
									error={errors.analogAdc1Mode}
									isInvalid={Boolean(errors.analogAdc1Mode)}
									onChange={handleChange}
								>
									{ANALOG_STICK_MODES.map((o, i) => (
										<option key={`button-analogAdc1Mode-option-${i}`} value={o.value}>
											{o.label}
										</option>
									))}
								</FormSelect>
								<FormSelect
									label={t('AddonsConfig:analog-adc-1-invert-label')}
									name="analogAdc1Invert"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.analogAdc1Invert}
									error={errors.analogAdc1Invert}
									isInvalid={Boolean(errors.analogAdc1Invert)}
									onChange={handleChange}
								>
									{INVERT_MODES.map((o, i) => (
										<option
											key={`button-analogAdc1Invert-option-${i}`}
											value={o.value}
										>
											{o.label}
										</option>
									))}
								</FormSelect>
							</Row>
							<Row className="mb-3">
								<FormControl
									type="number"
									label={t('AddonsConfig:inner-deadzone-size')}
									name="inner_deadzone"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.inner_deadzone}
									error={errors.inner_deadzone}
									isInvalid={Boolean(errors.inner_deadzone)}
									onChange={handleChange}
									min={0}
									max={100}
								/>
								<FormControl
									type="number"
									label={t('AddonsConfig:outer-deadzone-size')}
									name="outer_deadzone"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.outer_deadzone}
									error={errors.outer_deadzone}
									isInvalid={Boolean(errors.outer_deadzone)}
									onChange={handleChange}
									min={0}
									max={100}
								/>
							</Row>
							<Row className="mb-3">
								<FormCheck
									label={t('AddonsConfig:analog-smoothing')}
									type="switch"
									id="Analog_smoothing"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={Boolean(analogConfig.analog_smoothing)}
									onChange={(e) => {
										console.log(e);
										handleCheckbox('analog_smoothing');
										handleChange(e);
									}}
								/>
								<FormControl
									hidden={!analogConfig.analog_smoothing}
									type="number"
									label={t('AddonsConfig:smoothing-factor')}
									name="smoothing_factor"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.smoothing_factor}
									error={errors.smoothing_factor}
									isInvalid={Boolean(errors.smoothing_factor)}
									onChange={handleChange}
									min={0}
									max={100}
								/>
							</Row>
							<Row className="mb-3">
								<FormCheck
									label={t('AddonsConfig:analog-force-circularity')}
									type="switch"
									id="Forced_circularity"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={Boolean(analogConfig.forced_circularity)}
									onChange={(e) => {
										handleCheckbox('forced_circularity');
										handleChange(e);
									}}
								/>
								<FormSelect
									hidden={!analogConfig.forced_circularity}
									label={t('AddonsConfig:analog-error-label')}
									name="analog_error"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.analog_error}
									onChange={handleChange}
								>
									{ANALOG_ERROR_RATES.map((o, i) => (
										<option key={`analog_error-option-${i}`} value={o.value}>
											{o.label}
										</option>
									))}
								</FormSelect>
							</Row>
							<div className="d-flex align-items-center">
								<FormCheck
									label={t('AddonsConfig:analog-auto-calibrate')}
									type="switch"
									name="auto_calibrate"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={analogConfig.auto_calibrate}
									onChange={toggleConfig}
								/>
								<button
									type="button"
									className="btn btn-sm btn-outline-secondary ms-2"
									disabled={Boolean(analogConfig.auto_calibrate)}
									onClick={async () => {
										try {
											// Multi-step calibration process
											const calibrationValues = await getCalibrationValues({
													channels: analogConfig.analog_mux_channels,
													selectPins: [analogConfig.analogSelectPin0, analogConfig.analogSelectPin1, analogConfig.analogSelectPin2, analogConfig.analogSelectPin3],
													xChannel: analogConfig.analog_channel_x_1,
													xAdcPin: analogConfig.analogAdc1PinX,
													yChannel: analogConfig.analog_channel_y_1,
													yAdcPin: analogConfig.analogAdc1PinY
												});
											
											if (!calibrationValues) return;
											

											// Calculate center value from four points
											const avgX = Math.round(calibrationValues.reduce((sum, val) => sum + val.x, 0) / 4);
											const avgY = Math.round(calibrationValues.reduce((sum, val) => sum + val.y, 0) / 4);
											
											// Update joystick 1 center values
											setConfig('joystickCenterX', avgX);
											setConfig('joystickCenterY', avgY);
											
											console.log('Calibration completed:', {
												values: calibrationValues,
												finalCenter: { x: avgX, y: avgY }
											});
											
											
											// Show success message
											alert(
												t('AddonsConfig:analog-calibration-success-stick-1') + '\n\n' +
												t('AddonsConfig:analog-calibration-data') + '\n' +
												`• ${t('AddonsConfig:analog-calibration-direction-top-left')}: X=${calibrationValues[0].x}, Y=${calibrationValues[0].y}\n` +
												`• ${t('AddonsConfig:analog-calibration-direction-top-right')}: X=${calibrationValues[1].x}, Y=${calibrationValues[1].y}\n` +
												`• ${t('AddonsConfig:analog-calibration-direction-bottom-left')}: X=${calibrationValues[2].x}, Y=${calibrationValues[2].y}\n` +
												`• ${t('AddonsConfig:analog-calibration-direction-bottom-right')}: X=${calibrationValues[3].x}, Y=${calibrationValues[3].y}\n\n` +
												t('AddonsConfig:analog-calibration-final-center', { x: avgX, y: avgY }) + '\n\n' +
												t('AddonsConfig:analog-calibration-save-notice')
											);
										} catch (err) {
											console.error('Failed to calibrate joystick 1', err);
											alert(t('AddonsConfig:analog-calibration-failed', { error: err instanceof Error ? err.message : String(err) }));
										}
									}}
								>
									{t('AddonsConfig:analog-calibrate-stick-1-button')}
								</button>
								<div className="ms-3 small text-muted">
									{`Center: X=${analogConfig.joystickCenterX}, Y=${analogConfig.joystickCenterY}`}
								</div>
							</div>
							{Boolean(analogConfig.auto_calibrate) && (
								<div className="alert alert-info mt-2 mb-3">
									<small>
										<strong>{t('AddonsConfig:analog-auto-calibration-enabled-stick-1')}：</strong> {t('AddonsConfig:analog-calibration-auto-mode-instruction', { stick: '1' })}
									</small>
								</div>
							)}
							{!Boolean(analogConfig.auto_calibrate) && (
								<div className="alert alert-warning mt-2 mb-3">
									<small>
										<strong>{t('AddonsConfig:analog-manual-calibration-mode-stick-1')}：</strong> 
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-1')}
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-2')}
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-3')}
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-4')}
									</small>
								</div>
							)}
						</Row>
						</Col>
						<Col>
						<Row>
							<Button onClick={startVisualization}>Start</Button>
							<Button onClick={stopVisualization}>Stop</Button>
						</Row>

						{AnalogVisualization()}

						</Col>
						</Row>
					</Tab>
					<Tab
						key="analog2Config"
						eventKey="analog2Config"
						title={t('AddonsConfig:analog-adc-2')}
					>
						<Row className="mb-3">
							<FormCheck
								label={"use mux"}
								type="switch"
								id="use_mux"
								className="col-sm-3 ms-3"
								isInvalid={false}
								checked={Boolean(analogConfig.analog_mux_2)}
								onChange={(e) => {
									handleCheckbox('analog_mux_2');
									handleChange(e);
								}}
							/>
						</Row>
						{analogConfig.analog_mux_2 && <Row className="mb-3">
							<FormControl
								type="number"
								label={"x channel"}
								name="analog_channel_x_2"
								className="form-select-sm"
								groupClassName="col-sm-2 mb-3"
								value={analogConfig.analog_channel_x_2}
								error={errors.analog_channel_x_2}
								isInvalid={Boolean(errors.analog_channel_x_2)}
								onChange={handleChange}
								min={0}
								max={analogConfig.analog_mux_channels - 1}
							/>
							<FormControl
								type="number"
								label={"y channel"}
								name="analog_channel_y_2"
								className="form-select-sm"
								groupClassName="col-sm-2 mb-3"
								value={analogConfig.analog_channel_y_2}
								error={errors.analog_channel_y_2}
								isInvalid={Boolean(errors.analog_channel_y_2)}
								onChange={handleChange}
								min={0}
								max={analogConfig.analog_mux_channels - 1}
							/>
						</Row>}
						<Row className="mb-3">
							<FormSelect
								label={t('AddonsConfig:analog-adc-2-pin-x-label')}
								name="analogAdc2PinX"
								className="form-select-sm"
								groupClassName="col-sm-3 mb-3"
								value={analogConfig.analogAdc2PinX}
								error={errors.analogAdc2PinX}
								isInvalid={Boolean(errors.analogAdc2PinX)}
								onChange={handleChange}
							>
								<AnalogPinOptions />
							</FormSelect>
							<FormSelect
								label={t('AddonsConfig:analog-adc-2-pin-y-label')}
								name="analogAdc2PinY"
								className="form-select-sm"
								groupClassName="col-sm-3 mb-3"
								value={analogConfig.analogAdc2PinY}
								error={errors.analogAdc2PinY}
								isInvalid={Boolean(errors.analogAdc2PinY)}
								onChange={handleChange}
							>
								<AnalogPinOptions />
							</FormSelect>
							<Row className="mb-3">
								<FormSelect
									label={t('AddonsConfig:analog-adc-2-mode-label')}
									name="analogAdc2Mode"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.analogAdc2Mode}
									error={errors.analogAdc2Mode}
									isInvalid={Boolean(errors.analogAdc2Mode)}
									onChange={handleChange}
								>
									{ANALOG_STICK_MODES.map((o, i) => (
										<option key={`button-analogAdc2Mode-option-${i}`} value={o.value}>
											{o.label}
										</option>
									))}
								</FormSelect>
								<FormSelect
									label={t('AddonsConfig:analog-adc-2-invert-label')}
									name="analogAdc2Invert"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.analogAdc2Invert}
									error={errors.analogAdc2Invert}
									isInvalid={Boolean(errors.analogAdc2Invert)}
									onChange={handleChange}
								>
									{INVERT_MODES.map((o, i) => (
										<option
											key={`button-analogAdc2Invert-option-${i}`}
											value={o.value}
										>
											{o.label}
										</option>
									))}
								</FormSelect>
							</Row>
							<Row className="mb-3">
								<FormControl
									type="number"
									label={t('AddonsConfig:inner-deadzone-size')}
									name="inner_deadzone2"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.inner_deadzone2}
									error={errors.inner_deadzone2}
									isInvalid={Boolean(errors.inner_deadzone2)}
									onChange={handleChange}
									min={0}
									max={100}
								/>
								<FormControl
									type="number"
									label={t('AddonsConfig:outer-deadzone-size')}
									name="outer_deadzone2"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.outer_deadzone2}
									error={errors.outer_deadzone2}
									isInvalid={Boolean(errors.outer_deadzone2)}
									onChange={handleChange}
									min={0}
									max={100}
								/>
							</Row>
							<Row className="mb-3">
								<FormCheck
									label={t('AddonsConfig:analog-smoothing')}
									type="switch"
									id="Analog_smoothing2"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={Boolean(analogConfig.analog_smoothing2)}
									onChange={(e) => {
										handleCheckbox('analog_smoothing2');
										handleChange(e);
									}}
								/>
								<FormControl
									hidden={!analogConfig.analog_smoothing2}
									type="number"
									label={t('AddonsConfig:smoothing-factor')}
									name="smoothing_factor2"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.smoothing_factor2}
									error={errors.smoothing_factor2}
									isInvalid={Boolean(errors.smoothing_factor2)}
									onChange={handleChange}
									min={0}
									max={100}
								/>
							</Row>
							<Row className="mb-3">
								<FormCheck
									label={t('AddonsConfig:analog-force-circularity')}
									type="switch"
									id="Forced_circularity2"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={Boolean(analogConfig.forced_circularity2)}
									onChange={(e) => {
										handleCheckbox('forced_circularity2');
										handleChange(e);
									}}
								/>
								<FormSelect
									hidden={!analogConfig.forced_circularity2}
									label={t('AddonsConfig:analog-error-label')}
									name="analog_error2"
									className="form-control-sm"
									groupClassName="col-sm-3 mb-3"
									value={analogConfig.analog_error2}
									onChange={handleChange}
								>
									{ANALOG_ERROR_RATES.map((o, i) => (
										<option key={`analog_error-option-${i}`} value={o.value}>
											{o.label}
										</option>
									))}
								</FormSelect>
							</Row>
							<div className="d-flex align-items-center">
								<FormCheck
									label={t('AddonsConfig:analog-auto-calibrate')}
									type="switch"
									id="Auto_calibrate2"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={Boolean(analogConfig.auto_calibrate2)}
									onChange={(e) => {
										handleCheckbox('auto_calibrate2');
										handleChange(e);
									}}
								/>
								<button
									type="button"
									className="btn btn-sm btn-outline-secondary ms-2"
									disabled={Boolean(analogConfig.auto_calibrate2)}
									onClick={async () => {
										try {
											const calibrationValues = await getCalibrationValues({
													channels: analogConfig.analog_mux_channels,
													selectPins: [analogConfig.analogSelectPin0, analogConfig.analogSelectPin1, analogConfig.analogSelectPin2, analogConfig.analogSelectPin3],
													xChannel: analogConfig.analog_channel_x_2,
													xAdcPin: analogConfig.analogAdc2PinX,
													yChannel: analogConfig.analog_channel_y_2,
													yAdcPin: analogConfig.analogAdc2PinY
												});
											
											if (!calibrationValues) return;

											// Calculate center value from four points
											const avgX = Math.round(calibrationValues.reduce((sum, val) => sum + val.x, 0) / 4);
											const avgY = Math.round(calibrationValues.reduce((sum, val) => sum + val.y, 0) / 4);
											
											// Update joystick 2 center values
											setFieldValue('joystickCenterX2', avgX);
											setFieldValue('joystickCenterY2', avgY);
											
											console.log('Calibration completed:', {
												values: calibrationValues,
												finalCenter: { x: avgX, y: avgY }
											});
											
											
											// Show success message
											alert(
												t('AddonsConfig:analog-calibration-success-stick-2') + '\n\n' +
												t('AddonsConfig:analog-calibration-data') + '\n' +
												`• ${t('AddonsConfig:analog-calibration-direction-top-left')}: X=${calibrationValues[0].x}, Y=${calibrationValues[0].y}\n` +
												`• ${t('AddonsConfig:analog-calibration-direction-top-right')}: X=${calibrationValues[1].x}, Y=${calibrationValues[1].y}\n` +
												`• ${t('AddonsConfig:analog-calibration-direction-bottom-left')}: X=${calibrationValues[2].x}, Y=${calibrationValues[2].y}\n` +
												`• ${t('AddonsConfig:analog-calibration-direction-bottom-right')}: X=${calibrationValues[3].x}, Y=${calibrationValues[3].y}\n\n` +
												t('AddonsConfig:analog-calibration-final-center', { x: avgX, y: avgY }) + '\n\n' +
												t('AddonsConfig:analog-calibration-save-notice')
											);
										} catch (err) {
											console.error('Failed to calibrate joystick 2', err);
											alert(t('AddonsConfig:analog-calibration-failed', { error: err instanceof Error ? err.message : String(err) }));
										}
									}}
								>
									{t('AddonsConfig:analog-calibrate-stick-2-button')}
								</button>
								<div className="ms-3 small text-muted">
									{`Center: X=${analogConfig.joystickCenterX2}, Y=${analogConfig.joystickCenterY2}`}
								</div>
							</div>
							{Boolean(analogConfig.auto_calibrate2) && (
								<div className="alert alert-info mt-2 mb-3">
									<small>
										<strong>{t('AddonsConfig:analog-auto-calibration-enabled-stick-2')}：</strong> {t('AddonsConfig:analog-calibration-auto-mode-instruction', { stick: '2' })}
									</small>
								</div>
							)}
							{!Boolean(analogConfig.auto_calibrate2) && (
								<div className="alert alert-warning mt-2 mb-3">
									<small>
										<strong>{t('AddonsConfig:analog-manual-calibration-mode-stick-2')}：</strong> 
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-1')}
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-2')}
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-3')}
										<br />• {t('AddonsConfig:analog-calibration-manual-mode-instruction-4')}
									</small>
								</div>
							)}
						</Row>
					</Tab>
				</Tabs>
			</div>
			<FormCheck
				label={t('Common:switch-enabled')}
				type="switch"
				id="AnalogInputButton"
				reverse
				isInvalid={false}
				checked={Boolean(analogConfig.AnalogInputEnabled)}
				onChange={(e) => {
					handleCheckbox('AnalogInputEnabled');
					handleChange(e);
				}}
			/>
		</Section>
	);
};

export default Analog;
