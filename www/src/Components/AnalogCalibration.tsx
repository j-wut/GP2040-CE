import * as d3 from "d3";
import React, { useEffect, useState, useRef, ReactElement } from 'react';
import { Button, Modal, Row, Col, ProgressBar, Form, Spinner, Dropdown } from 'react-bootstrap';
import { useTranslation } from 'react-i18next';

import './HECalibration.scss';

import { BUTTON_ACTIONS } from '../Data/Pins';
import invert from 'lodash/invert';
import { ADC_MAX, AnalogCalibrationPoint, AnalogInvertMode, AnalogOptions, AnalogPluginOptions } from '../Data/Types';
import FormSelect from './FormSelect';
import { number } from 'yup';
import WebApi from '../Services/WebApi';

type AnalogCalibrationProps = {
	pluginConfig: AnalogPluginOptions;
	options: AnalogOptions;
	showCalibration: boolean;
	hideCalibration: () => void;
	saveCalibration: (calibrationPoints: AnalogCalibrationPoint[], invertMode: AnalogInvertMode) => void;
}


const INVERT_MODES = [
	{ label: 'None', value: AnalogInvertMode.NONE},
	{ label: 'X Axis', value: AnalogInvertMode.X_AXIS },
	{ label: 'Y Axis', value: AnalogInvertMode.Y_AXIS },
	{ label: 'X/Y Axis', value: AnalogInvertMode.XY_AXIS },
];

const AnalogCalibration = ({
	pluginConfig,
	options,
	showCalibration,
	hideCalibration,
	saveCalibration
}: AnalogCalibrationProps) => {
	const { t } = useTranslation('');

	const [calibrationPoints, setCalibrationPoints] = useState<AnalogCalibrationPoint[]>([]);
	const [invertMode, setInvertMode] = useState<AnalogInvertMode>(AnalogInvertMode.NONE);

	
	const [selectedPosition, setSelectedPosition] = useState(5);
	const timerId = useRef<number>();
	const [joystickPosition, setJoystickPosition] = useState({x:0,y:0});
	const [correctedPosition, setCorrectedPosition] = useState({x:0,y:0});
	
	const POSITION_OPTIONS = {
		1: "Down Left",
		2: "Down",
		3: "Down Right",
		4: "Left",
		5: "Neutral",
		6: "Right",
		7: "Up Left",
		8: "Up",
		9: "Up Right",
	};

	const appendCalibrationPoint = () => {
		let temporary_calibrations:AnalogCalibrationPoint[] = [...calibrationPoints];

		let notchX: number;
		let notchY: number;
		if (selectedPosition % 3 == 1) {
			notchX = 0;
		} else if (selectedPosition % 3 == 0){
			notchX = ADC_MAX;
		} else {
			notchX = ADC_MAX/2;
		}
		if (selectedPosition <= 3) {
			notchY = ADC_MAX;
		} else if (selectedPosition > 6){
			notchY = 0;
		} else {
			notchY = ADC_MAX/2;
		}

		temporary_calibrations[selectedPosition-1] = {source:joystickPosition, target: {x: notchX, y: notchY}};
		setCalibrationPoints(temporary_calibrations);
	}

	useEffect(()=>{
		setInvertMode(options.invert_mode);
		startJoystickPolling();
	},[])

	const cancelCalibration=()=>{
		stopJoystickPolling();
		hideCalibration();
	}

	useEffect(()=>{
		stopJoystickPolling();
		startJoystickPolling();
	},[invertMode])

	useEffect(()=>{

		if (Object.values(calibrationPoints).length === 0) {
			setCorrectedPosition(joystickPosition);
			return;
		};

        // Calculate influence of each control point
        let weights: number[] = [];
        let totalWeight = 0.0;

		Object.values(calibrationPoints).forEach((entry, i)=>{
			const distance = Math.sqrt((joystickPosition.x - entry.source.x)**2 + (joystickPosition.y - entry.source.y)**2)
			if (distance <= 0.0) {
                weights[i] = 1e6; // Large weight for exact matches
            } else {
                weights[i] = 1.0 / (distance**2);
            }
            totalWeight += weights[i];
		})

		weights = weights.map((w)=>w/=totalWeight);

        // Apply weighted average of warping transformations
        let resultX = 0.0;
        let resultY = 0.0;
        
		Object.values(calibrationPoints).forEach((entry, i)=>{
			const weight = weights[i];

			resultX += weight * (entry.target.x + joystickPosition.x - entry.source.x)
			resultY += weight * (entry.target.y + joystickPosition.y - entry.source.y)
		})

		setCorrectedPosition({x: resultX, y:resultY})

	}, [joystickPosition])

	const readJoystickPosition = async () => {
		let position = await WebApi.getJoystickPosition({
			channels: pluginConfig.analog_mux_channels,
			selectPins: [pluginConfig.analogSelectPin0, pluginConfig.analogSelectPin1, pluginConfig.analogSelectPin2, pluginConfig.analogSelectPin3],
			xChannel: options.channel_x,
			xAdcPin: options.pin_x,
			yChannel: options.channel_y,
			yAdcPin: options.pin_y
		});
		if (invertMode == AnalogInvertMode.XY_AXIS || invertMode == AnalogInvertMode.X_AXIS) {
			position.x = ADC_MAX - position.x;
		}
		if (invertMode == AnalogInvertMode.XY_AXIS || invertMode == AnalogInvertMode.Y_AXIS) {
			position.y = ADC_MAX - position.y;
		}
		setJoystickPosition(position);
	}

	const stopJoystickPolling = () => {
		if (timerId)
			clearInterval(timerId.current);
	};

	const startJoystickPolling = () => {
		if (timerId.current)
			clearInterval(timerId.current);
		const intervalId = setInterval(() => {
			readJoystickPosition();
		}, 50);
		timerId.current = intervalId;
	}

	const AnalogVisualization = (): ReactElement => {
		let directions = options.snap_directions;
		let pathD = directions.reverse().map((d) =>d3.arc().innerRadius(d.activation).outerRadius(d.release).startAngle(d.angle-d.snap_margin).endAngle(d.angle+d.snap_margin)());

		return (
			<svg viewBox={`-100 -100 200 200`}> {/* at some point have to figure out why this is dumb as fuck */}
				<circle fill="white" stroke="black" strokeWidth="1" cx="0" cy="0" r={100}/>
				{
				pathD.map((d, i) => <path x="0" y="0"  fillOpacity="40%" strokeWidth="2" d={d}/>)
				}
				<circle fill="black" fillOpacity="50%" cx={(joystickPosition.x - ADC_MAX/2)/(ADC_MAX/2)*100} cy={(joystickPosition.y - ADC_MAX/2)/(ADC_MAX/2)*100} r={3}/>
				<circle fill="crimson" fillOpacity="100%" cx={(correctedPosition.x - ADC_MAX/2)/(ADC_MAX/2)*100} cy={(correctedPosition.y - ADC_MAX/2)/(ADC_MAX/2)*100} r={3}/>
			</svg>
		);
	}

	return (
		<>
			<Modal className="modal-lg" contentClassName="he-modal" centered show={showCalibration}
				onClose={() => cancelCalibration()}
				onHide={() => cancelCalibration()}
			>
				<Modal.Header closeButton>
					<Modal.Title className="me-auto">Analog Calibration</Modal.Title>
				</Modal.Header>
				<Modal.Body>
					{AnalogVisualization()}
					<FormSelect
							label="position"
							name="current_position"
							className="form-select-sm"
							groupClassName="col-sm-3 mb-3"
							value={selectedPosition}
							onChange={(e)=>{
								setSelectedPosition(+e.target.value)
							}}
						>
							{Object.entries(POSITION_OPTIONS).map(([num, label]) => (
								<option key={`calibrating-position-${num}`} value={num}>
									{label}
								</option>
							))}
						</FormSelect>
						<FormSelect
							label={t('AddonsConfig:analog-adc-1-invert-label')}
							name="analogAdc1Invert"
							className="form-select-sm"
							groupClassName="col-sm-3 mb-3"
							value={invertMode}
							onChange={(e)=>{
								console.log(e.target.value)
								setInvertMode(+e.target.value)
							}}
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
				</Modal.Body>
				<Modal.Footer>
					<Button
						onClick={appendCalibrationPoint}>
						Add Calibration Point
					</Button>
					<Button onClick={()=>{
						saveCalibration(calibrationPoints ,invertMode)
					}}>
						Save
					</Button>
				</Modal.Footer>
			</Modal>
		</>
	);
};

export default AnalogCalibration;
