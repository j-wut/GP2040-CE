
import { ChangeEvent, useContext, useEffect, useRef, useState } from 'react';
import { useTranslation } from 'react-i18next';
import { Button, Col, FormCheck, Row, Tab, Tabs } from 'react-bootstrap';

import Section from '../Components/Section';
import FormSelect from '../Components/FormSelect';
import { ANALOG_PINS } from '../Data/Buttons';
import AnalogPinOptions from '../Components/AnalogPinOptions';
import { AppContext } from '../Contexts/AppContext';
import FormControl from '../Components/FormControl';
import { AddonPropTypes } from '../Pages/AddonsConfigPage';
import { AnalogCalibrationPoint, AnalogInvertMode, AnalogMode, AnalogPlusOptions } from '../Data/Types';
import WebApi from '../Services/WebApi';
import AnalogPlusCalibration from "../Components/AnalogPlusCalibration";

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

export const analogPlusDefaultState: AnalogPlusOptions = {
	enabled: false,

	mux_channels: 0,
	select_pins: [],
	
	analog_configs: [{
		analog_mode: 1,
		pin_x: 0,
		pin_y: 0,
		invert_mode: 0,
		forced_circularity: false,
		inner_deadzone: 0,
		outer_deadzone: 95,
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
		analog_mode: 2,
		pin_x: 0,
		pin_y: 0,
		invert_mode: 0,
		forced_circularity: false,
		inner_deadzone: 0,
		outer_deadzone: 95,
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

const AnalogPlus = ({  }: AddonPropTypes) => {
	const [analogPlusOptions , setAnalogPlusOptions] = useState(analogPlusDefaultState);
	const { usedPins } = useContext(AppContext);
	const { t } = useTranslation();

	const saveCalibration = (index: number, calibrationPoints: AnalogCalibrationPoint[], invertMode: AnalogInvertMode) => {

		analogPlusOptions.analog_configs[index].calibration_points = calibrationPoints;
		analogPlusOptions.analog_configs[index].invert_mode = invertMode;

		setAnalogPlusOptions(analogPlusOptions);
	}

	const setAnalogConfig = (index: number, fieldName: string , value: any) => {

		const temp: AnalogPlusOptions = {...analogPlusOptions};
		temp.analog_configs[index][fieldName] = value;
		setAnalogPlusOptions(temp)
	}

	
	const setAddonField = (fieldName: string , value: any) => {
		const temp: AnalogPlusOptions = {...analogPlusOptions}
		temp[fieldName] = value;
		setAnalogPlusOptions(temp)
	}

	const handleChange = (e:ChangeEvent<FormControlElement>) => setAddonField(e.target.name, e.target.value);
	const handleCheckbox = (e:ChangeEvent<FormControlElement>, value: boolean) => setAddonField(e.target.id, value);

	const setSelectPin = (index: number, pin: number) => {
		const temp = {...analogPlusOptions};
		temp.select_pins[index] = pin;
		setAnalogPlusOptions(temp);
	}


	useEffect(()=>{
		WebApi.getAnalogPlusSettings().then(
			setAnalogPlusOptions
		)
	}, [])

	
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
				Analog <em style={{fontSize:18}}>PLUS</em>
			</a>
		}
		>
			<div id="AnalogInputOptions" hidden={!analogPlusOptions.enabled}>
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
						name="mux_channels"
						className="form-select-sm"
						groupClassName="col-sm-3 mb-3"
						value={analogPlusOptions.mux_channels}
						// error={errors.analog_mux_channels}
						// isInvalid={Boolean(errors.analog_mux_channels)}
						onChange={
							(e) => setAddonField(e.target.name, +e.target.value)
						}
					>
						{Object.entries(CHANNELS_OPTIONS).map(([num, label], i) => (
							<option key={`channels-per-mux-option-${i}`} value={num}>
								{label}
							</option>
						))}
					</FormSelect>
				</Row>
				<Row className="mb-3">
					{analogPlusOptions.mux_channels > 1 && [...Array(Math.log2(analogPlusOptions.mux_channels)).keys()].map((_,i)=> {
							return <FormControl
								type="number"
								label={`Select Pin ${i+1}`}
								name={`analogSelectPin${i}`}
								className="form-select-sm"
								groupClassName="col-sm-2 mb-3"
								value={analogPlusOptions.select_pins?.length ? analogPlusOptions.select_pins[i] : -1}
								// error={errors.analogSelectPin1}
								// isInvalid={Boolean(errors.analogSelectPin1)}
								onChange={(e)=>{
									setSelectPin(i, +e.currentTarget.value)}}
								min={-1}
								max={29}
							/>
						})
					}
				</Row>

				<Tabs
					defaultActiveKey="analog1Config"
					id="analogConfigTabs"
					className="mb-3 pb-0"
					fill
				>
					{
						analogPlusOptions.analog_configs.map((config, index)=> {
							const handleChange = (e:ChangeEvent<FormControlElement>) => setAnalogConfig(index, e.target.name, e.target.value);
							const handleCheckbox = (e:ChangeEvent<FormControlElement>, value: boolean) => setAnalogConfig(index, e.target.id, value);
							const [showCalibration, setShowCalibration] = useState(false);

							return (
						<Tab key={`analog${index+1}Config`}
								eventKey={`analog${index+1}Config`}
							title={`Analog Stick ${index+1}`}>
							{showCalibration && <>
							<AnalogPlusCalibration 
									showCalibration={showCalibration}
									pluginOptions={analogPlusOptions}
									config={config}
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
									checked={Boolean(config.use_mux)}
									onChange={(e) => {
										handleCheckbox(e, !config.use_mux)
									}}
								/>
							</Row>
							{config.use_mux && <Row className="mb-3">
								<FormControl
									type="number"
									label={"x channel"}
									name="channel_x"
									className="form-select-sm"
									groupClassName="col-sm-2 mb-3"
									value={config.channel_x}
									// error={errors.analog_channel_x_1}
									// isInvalid={Boolean(errors.analog_channel_x_1)}
									onChange={handleChange}
									min={0}
									max={analogPlusOptions.mux_channels - 1}
								/>
								<FormControl
									type="number"
									label={"y channel"}
									name="channel_y"
									className="form-select-sm"
									groupClassName="col-sm-2 mb-3"
									value={config.channel_y}
									// error={errors.analog_channel_y_1}
									// isInvalid={Boolean(errors.analog_channel_y_1)}
									onChange={handleChange}
									min={0}
									max={analogPlusOptions.mux_channels - 1}
								/>
							</Row>}
							<Row className="mb-3">
								<FormSelect
									label={t(`AddonsConfig:analog-adc-${index}-pin-x-label`)}
									name="pin_x"
									className="form-select-sm"
									groupClassName="col-sm-3 mb-3"
									value={config.pin_x}
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
									value={config.pin_y}
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
										value={config.analog_mode}
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
										value={config.inner_deadzone}
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
										value={config.outer_deadzone}
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
										checked={config.analog_smoothing}
										onChange={(e) => {
											handleCheckbox(e, !config.analog_smoothing)
										}}
									/>
									<FormControl
										hidden={!analogPlusOptions.analog_smoothing}
										type="number"
										label={t('AddonsConfig:smoothing-factor')}
										name="smoothing_factor"
										className="form-control-sm"
										groupClassName="col-sm-3 mb-3"
										value={config.smoothing_factor}
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
										checked={config.forced_circularity}
										onChange={(e) => {
											handleCheckbox(e, !config.forced_circularity);
										}}
									/>
									<FormSelect
										hidden={!analogPlusOptions.forced_circularity}
										label={t('AddonsConfig:analog-error-label')}
										name="analog_error"
										className="form-control-sm"
										groupClassName="col-sm-3 mb-3"
										value={config.analog_error}
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
										checked={config.auto_calibrate}
										onChange={(e)=>{
											handleCheckbox(e, !config.auto_calibrate)
										}}
									/>
									<button
										type="button"
										className="btn btn-sm btn-outline-secondary ms-2"
										disabled={Boolean(config.auto_calibrate)}
										onClick={()=>setShowCalibration(true)}
									>
										{t('AddonsConfig:analog-calibrate-stick-1-button')}
									</button>
								</div>
								{Boolean(config.auto_calibrate) && (
									<div className="alert alert-info mt-2 mb-3">
										<small>
											<strong>{t('AddonsConfig:analog-auto-calibration-enabled-stick-1')}：</strong> {t('AddonsConfig:analog-calibration-auto-mode-instruction', { stick: '1' })}
										</small>
									</div>
								)}
								{!Boolean(config.auto_calibrate) && (
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
			<Button onClick={async () => await WebApi.setAnalogPlusOptions(analogPlusOptions)}>
				{t('Common:button-save-label')}
			</Button>
			<FormCheck
				label={t('Common:switch-enabled')}
				type="switch"
				id="enabled"
				reverse
				isInvalid={false}
				checked={Boolean(analogPlusOptions.enabled)}
				onChange={(e) => {
					handleCheckbox(e, !analogPlusOptions.enabled)
				}}
			/>
		</Section>
	);
};

export default AnalogPlus;
