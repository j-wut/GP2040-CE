
import { ChangeEvent, ReactElement, useContext, useEffect, useRef, useState } from 'react';
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
import { ADC_MAX, AnalogCalibrationPoint, AnalogInvertMode, AnalogMode, AnalogOptions, AnalogPluginOptions, AnalogSnapDirection, GetJoystickPositionRequest, JoystickPosition } from '../Data/Types';
import WebApi from '../Services/WebApi';
import { set } from "lodash";
import AnalogCalibration from "../Components/AnalogCalibration";
import { optionCSS } from 'react-select/dist/declarations/src/components/Option';
import { index } from 'd3';

const ANALOG_STICK_MODES = [
	{ label: 'Left Analog', value: AnalogMode.LEFT_ANALOG },
	{ label: 'Right Analog', value: AnalogMode.RIGHT_ANALOG },
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

export const analogState: AnalogPluginOptions = {
	AnalogInputEnabled: false,

	analog_mux_channels: 0,
	analogSelectPin0: -1,
	analogSelectPin1: -1,
	analogSelectPin2: -1,
	analogSelectPin3: -1,
	
	analogOptions: [{
		analog_mode: AnalogMode.DISABLED,
		pin_x: 0,
		pin_y: 0,
		invert_mode: AnalogInvertMode.NONE,
		forced_circularity: false,
		inner_deadzone: 0,
		auto_calibrate: false,
		calibration_points: [],
		analog_smoothing: false,
		smoothing_factor: 0,
		analog_error: 0,
		use_mux: false,
		channel_x: 0,
		channel_y: 0,
		linearity: 0,
		angle_snapping: false,
		snap_directions: []
	},{
		analog_mode: AnalogMode.DISABLED,
		pin_x: 0,
		pin_y: 0,
		invert_mode: AnalogInvertMode.NONE,
		forced_circularity: false,
		inner_deadzone: 0,
		auto_calibrate: false,
		calibration_points: [],
		analog_smoothing: false,
		smoothing_factor: 0,
		analog_error: 0,
		use_mux: false,
		channel_x: 0,
		channel_y: 0,
		linearity: 0,
		angle_snapping: false,
		snap_directions: []
	}],
};

const Analog = ({  }: AddonPropTypes) => {
	const [analogConfig , setAnalogConfig] = useState(analogState);
	const { usedPins } = useContext(AppContext);
	const { t } = useTranslation();

	const saveCalibration = (index: number, calibrationPoints: AnalogCalibrationPoint[], invertMode: AnalogInvertMode) => {

		analogConfig.analogOptions[index].calibration_points = calibrationPoints;
		analogConfig.analogOptions[index].invert_mode = invertMode;

		setAnalogConfig(analogConfig);
	}

	const setAnalogOption = (index: number, fieldName: string , value: any) => {
		const temp = {...analogConfig};
		temp.analogOptions[index][fieldName] = value;
		setAnalogConfig(temp)
	}

	
	const setPluginConfig = (fieldName: string , value: any) => {
		const temp = {...analogConfig}
		temp[fieldName] = value;
		setAnalogConfig(temp)
	}

	const handleChange = (e:ChangeEvent<FormControlElement>) => setPluginConfig(e.target.name, e.target.value);
	const handleCheckbox = (e:ChangeEvent<FormControlElement>, value: boolean) => setPluginConfig(e.target.id, value);


	// useEffect(()=>{
	// 	const getAnalogConfig = async ()=> await WebApi.getAnalogSettings();
	// 	getAnalogConfig().then(
	// 		setAnalogConfig
	// 	);
	// }, [])

	
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
				
				<Row className="mt-2">
					<FormSelect
						label={t('HETrigger:multiplexer-channel-select')}
						name="analog_mux_channels"
						className="form-select-sm"
						groupClassName="col-sm-3 mb-3"
						value={analogConfig.analog_mux_channels}
						// error={errors.analog_mux_channels}
						// isInvalid={Boolean(errors.analog_mux_channels)}
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
						// error={errors.analogSelectPin0}
						// isInvalid={Boolean(errors.analogSelectPin0)}
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
						// error={errors.analogSelectPin1}
						// isInvalid={Boolean(errors.analogSelectPin1)}
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
						// error={errors.analogSelectPin2}
						// isInvalid={Boolean(errors.analogSelectPin2)}
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
						// error={errors.analogSelectPin3}
						// isInvalid={Boolean(errors.analogSelectPin3)}
						onChange={handleChange}
						min={-1}
						max={29}
					/>
				</Row>

				<Tabs
					defaultActiveKey="analog1Config"
					id="analogConfigTabs"
					className="mb-3 pb-0"
					fill
				>
					{
						analogConfig.analogOptions.map((options, index)=> {
							const handleChange = (e:ChangeEvent<FormControlElement>) => setAnalogOption(index, e.target.name, e.target.value);
							const handleCheckbox = (e:ChangeEvent<FormControlElement>, value: boolean) => setAnalogOption(index, e.target.id, value);
							const [showCalibration, setShowCalibration] = useState(false);

							return (
						<Tab key={`analog${index+1}Config`}
								eventKey={`analog${index+1}Config`}
							title={`Analog Stick ${index+1}`}>
							{showCalibration && <>
							<AnalogCalibration 
									showCalibration={showCalibration}
									pluginConfig={analogConfig}
									options={options}
									hideCalibration={() => setShowCalibration(false)}
									saveCalibration={(calibrationPoints: AnalogCalibrationPoint[], invertMode: AnalogInvertMode) => saveCalibration(index, calibrationPoints, invertMode)}/>
									</>}

							<Row>
							<Col>
							<Row className="mb-3">
								<FormCheck
									label={"use mux"}
									type="switch"
									id="use_mux"
									className="col-sm-3 ms-3"
									isInvalid={false}
									checked={Boolean(options.use_mux)}
									onChange={(e) => {
										handleCheckbox(e, !options.use_mux)
									}}
								/>
							</Row>
							{analogConfig.analog_mux_1 && <Row className="mb-3">
								<FormControl
									type="number"
									label={"x channel"}
									name="channel_x"
									className="form-select-sm"
									groupClassName="col-sm-2 mb-3"
									value={options.channel_x}
									// error={errors.analog_channel_x_1}
									// isInvalid={Boolean(errors.analog_channel_x_1)}
									onChange={handleChange}
									min={0}
									max={analogConfig.analog_mux_channels - 1}
								/>
								<FormControl
									type="number"
									label={"y channel"}
									name="channel_y"
									className="form-select-sm"
									groupClassName="col-sm-2 mb-3"
									value={options.channel_y}
									// error={errors.analog_channel_y_1}
									// isInvalid={Boolean(errors.analog_channel_y_1)}
									onChange={handleChange}
									min={0}
									max={analogConfig.analog_mux_channels - 1}
								/>
							</Row>}
							<Row className="mb-3">
								<FormSelect
									label={t(`AddonsConfig:analog-adc-${index}-pin-x-label`)}
									name="pin_x"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={options.pin_x}
									// error={errors.analogAdc1PinX}
									// isInvalid={Boolean(errors.analogAdc1PinX)}
									onChange={handleChange}
								>
									<AnalogPinOptions />
								</FormSelect>
								<FormSelect
									label={t('AddonsConfig:analog-adc-1-pin-y-label')}
									name="pin_y"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={options.pin_y}
									// error={errors.analogAdc1PinY}
									// isInvalid={Boolean(errors.analogAdc1PinY)}
									onChange={handleChange}
								>
									<AnalogPinOptions />
								</FormSelect>
								<Row className="mb-3">
									<FormSelect
										label={t('AddonsConfig:analog-adc-1-mode-label')}
										name="analog_mode"
										className="form-select-sm"
										groupClassName="col-sm-3 mb-3"
										value={options.analog_mode}
										// error={errors.analogAdc1Mode}
										// isInvalid={Boolean(errors.analogAdc1Mode)}
										onChange={handleChange}
									>
										{ANALOG_STICK_MODES.map((o, i) => (
											<option key={`button-analogAdc1Mode-option-${i}`} value={o.value}>
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
										value={options.inner_deadzone}
										// error={errors.inner_deadzone}
										// isInvalid={Boolean(errors.inner_deadzone)}
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
										value={options.outer_deadzone}
										// error={errors.outer_deadzone}
										// isInvalid={Boolean(errors.outer_deadzone)}
										onChange={handleChange}
										min={0}
										max={100}
									/>
								</Row>
								<Row className="mb-3">
									<FormCheck
										label={t('AddonsConfig:analog-smoothing')}
										type="switch"
										id="analog_smoothing"
										className="col-sm-3 ms-3"
										isInvalid={false}
										checked={options.analog_smoothing}
										onChange={(e) => {
											handleCheckbox(e, !options.analog_smoothing)
										}}
									/>
									<FormControl
										hidden={!analogConfig.analog_smoothing}
										type="number"
										label={t('AddonsConfig:smoothing-factor')}
										name="smoothing_factor"
										className="form-control-sm"
										groupClassName="col-sm-3 mb-3"
										value={options.smoothing_factor}
										// error={errors.smoothing_factor}
										// isInvalid={Boolean(errors.smoothing_factor)}
										onChange={handleChange}
										min={0}
										max={100}
									/>
								</Row>
								<Row className="mb-3">
									<FormCheck
										label={t('AddonsConfig:analog-force-circularity')}
										type="switch"
										id="forced_circularity"
										className="col-sm-3 ms-3"
										isInvalid={false}
										checked={options.forced_circularity}
										onChange={(e) => {
											handleCheckbox(e, !options.forced_circularity);
										}}
									/>
									<FormSelect
										hidden={!analogConfig.forced_circularity}
										label={t('AddonsConfig:analog-error-label')}
										name="analog_error"
										className="form-control-sm"
										groupClassName="col-sm-3 mb-3"
										value={options.analog_error}
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
										id="auto_calibrate"
										className="col-sm-3 ms-3"
										isInvalid={false}
										checked={options.auto_calibrate}
										onChange={(e)=>{
											handleCheckbox(e, !options.auto_calibrate)
										}}
									/>
									<button
										type="button"
										className="btn btn-sm btn-outline-secondary ms-2"
										disabled={Boolean(options.auto_calibrate)}
										onClick={()=>setShowCalibration(true)}
									>
										{t('AddonsConfig:analog-calibrate-stick-1-button')}
									</button>
								</div>
								{Boolean(options.auto_calibrate) && (
									<div className="alert alert-info mt-2 mb-3">
										<small>
											<strong>{t('AddonsConfig:analog-auto-calibration-enabled-stick-1')}：</strong> {t('AddonsConfig:analog-calibration-auto-mode-instruction', { stick: '1' })}
										</small>
									</div>
								)}
								{!Boolean(options.auto_calibrate) && (
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
							</Row>
						</Tab>
						)})
					}
				</Tabs>
			</div>
			<FormCheck
				label={t('Common:switch-enabled')}
				type="switch"
				id="AnalogInputEnabled"
				reverse
				isInvalid={false}
				checked={Boolean(analogConfig.AnalogInputEnabled)}
				onChange={(e) => {
					handleCheckbox(e, !analogConfig.AnalogInputEnabled)
				}}
			/>
		</Section>
	);
};

export default Analog;
