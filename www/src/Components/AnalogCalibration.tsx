import * as d3 from "d3";
import React, { useEffect, useState, useRef, ReactElement } from 'react';
import { Button, Modal, Row, Col, ProgressBar, Form, Spinner, Dropdown } from 'react-bootstrap';
import { useTranslation } from 'react-i18next';

import './HECalibration.scss';

import { BUTTON_ACTIONS } from '../Data/Pins';
import invert from 'lodash/invert';
import { ADC_MAX, AnalogInvertMode, AnalogOptions, JoystickPosition } from '../Data/Types';
import FormSelect from './FormSelect';
import { number } from 'yup';
import WebApi from '../Services/WebApi';
import { result } from "lodash";

type AnalogCalibrationProps = {
	analogConfig: AnalogOptions;
	showCalibration: boolean;
	hideCalibration: () => void;
	saveCalibrationPoints: (calibrationPoints: [[number, number],[number,number]][]) => void;
}



interface AnalogCalibrationPoints {
	[key: number]: [[number, number],[number,number]];
}

const AnalogCalibration = ({
	analogConfig,
	showCalibration,
	hideCalibration,
	saveCalibrationPoints,
}: AnalogCalibrationProps) => {
	const { t } = useTranslation('');

	const [calibrationPoints, setCalibrationPoints] = useState<AnalogCalibrationPoints>({});
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
	
	const saveCalibration = () => {
		saveCalibrationPoints(Object.values(calibrationPoints));
		cancelCalibration();
	}

	const appendCalibrationPoint = () => {
		let temporary_calibrations:AnalogCalibrationPoints = {...calibrationPoints};

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

		temporary_calibrations[selectedPosition] = [[joystickPosition.x, joystickPosition.y],[notchX, notchY]];
		setCalibrationPoints(temporary_calibrations);
	}

	useEffect(()=>{
		startJoystickPolling();
	},[])

	const cancelCalibration=()=>{
		stopJoystickPolling();
		hideCalibration();
	}

	useEffect(()=>{

		if (Object.values(calibrationPoints).length === 0) {
			setCorrectedPosition(joystickPosition);
			return;
		};

        // Calculate influence of each control point
        let weights: number[] = [];
        let totalWeight = 0.0;

		Object.values(calibrationPoints).forEach((entry, i)=>{
			const [[sourceX, sourceY], _] = entry
			const distance = Math.sqrt((joystickPosition.x - sourceX)**2 + (joystickPosition.y - sourceY)**2)
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
			const [[sourceX, sourceY], [targetX,targetY]] = entry

			resultX += weight * (targetX + joystickPosition.x - sourceX)
			resultY += weight * (targetY + joystickPosition.y - sourceY)
		})

		setCorrectedPosition({x: resultX, y:resultY})

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
		let directions = analogConfig.analog_directions_1;
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
				</Modal.Body>
				<Modal.Footer>
					<Button
						onClick={appendCalibrationPoint}>
						Add Calibration Point
					</Button>
					<Button onClick={saveCalibration}>
						Save
					</Button>
				</Modal.Footer>
			</Modal>
		</>
	);
};

export default AnalogCalibration;
